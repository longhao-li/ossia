#include "ossia/io_context.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <WinSock2.h>
#    include <Windows.h>
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
#    include <liburing.h>
#    include <sys/utsname.h>
#else
#    error "Unsupported operating system"
#endif

#include <cassert>
#include <system_error>
#include <thread>

using namespace ossia;
using namespace ossia::detail;

/// \brief
///   Current worker for the calling thread.
static thread_local io_context_worker *current_worker;

#if defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
/// \brief
///   Create an unsigned int that represents a version number.
/// \param major
///   Major linux kernel version number.
/// \param minor
///   Minor linux kernel version number.
/// \param patch
///   Patch linux kernel version number.
[[nodiscard]]
static auto make_version(std::uint8_t major, std::uint8_t minor, std::uint8_t patch) noexcept
    -> std::uint32_t {
    return (static_cast<std::uint32_t>(major) << 16) | (static_cast<std::uint32_t>(minor) << 8) |
           patch;
}

/// \brief
///   Get current linux kernel version. This is used to check if certain \c io_uring features are
///   supported.
/// \return
///   An unsigned integer that represents current linux kernel version. This is created via function
///   \c make_version.
[[nodiscard]]
static auto kernel_version() noexcept -> std::uint32_t {
    std::uint8_t versions[3]{};

    struct utsname name;
    if (::uname(&name) != 0)
        return 0;

    std::string_view s = name.release;
    std::uint8_t    *v = versions;

    for (char c : s) {
        if (c >= '0' && c <= '9')
            *v = *v * 10 + static_cast<std::uint8_t>(c) - '0';
        else if (c == '.')
            ++v;
        else
            break;

        if (v >= versions + std::size(versions)) [[unlikely]]
            break;
    }

    return make_version(versions[0], versions[1], versions[2]);
}

/// \brief
///   Get available \c io_uring setup flags according to current kernel version.
/// \return
///   Available \c io_uring setup flags.
[[nodiscard]]
static auto io_uring_setup_flags() noexcept -> std::uint32_t {
    std::uint32_t flags   = IORING_SETUP_CLAMP;
    std::uint32_t version = kernel_version();

    if (version >= make_version(5, 18, 0))
        flags |= IORING_SETUP_SUBMIT_ALL;

    if (version >= make_version(5, 19, 0)) {
        flags |= IORING_SETUP_COOP_TASKRUN;
        flags |= IORING_SETUP_TASKRUN_FLAG;
    }

    if (version >= make_version(6, 0, 0))
        flags |= IORING_SETUP_SINGLE_ISSUER;

    return flags;
}

/// \brief
///   Get available \c io_uring feature flags according to current kernel version.
/// \return
///   Available \c io_uring feature flags.
[[nodiscard]]
auto io_uring_setup_features() noexcept -> std::uint32_t {
    std::uint32_t features = 0;
    std::uint32_t version  = kernel_version();

    if (version >= make_version(5, 4, 0))
        features |= IORING_FEAT_SINGLE_MMAP;

    if (version >= make_version(5, 5, 0))
        features |= IORING_FEAT_NODROP;

    if (version >= make_version(5, 6, 0))
        features |= IORING_FEAT_RW_CUR_POS;

    if (version >= make_version(5, 7, 0))
        features |= IORING_FEAT_FAST_POLL;

    return features;
}
#endif

io_context_worker::io_context_worker()
    : m_is_running(),
      m_thread_id(),
      m_muxer(),
      m_tasks(),
      m_should_stop() {
    m_tasks.reserve(64);

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    m_muxer = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (m_muxer == nullptr) [[unlikely]]
        throw std::system_error(GetLastError(), std::system_category(), "Failed to create IOCP");
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    io_uring *ring = static_cast<io_uring *>(std::malloc(sizeof(io_uring)));
    assert(ring != nullptr);

    io_uring_params params{
        .sq_entries     = 0,
        .cq_entries     = 0,
        .flags          = io_uring_setup_flags(),
        .sq_thread_cpu  = 0,
        .sq_thread_idle = 0,
        .features       = io_uring_setup_features(),
        .wq_fd          = 0,
        .resv           = {},
        .sq_off         = {},
        .cq_off         = {},
    };

    int result = io_uring_queue_init_params(32768, ring, &params);
    if (result != 0) [[unlikely]] {
        std::free(ring);
        throw std::system_error(-result, std::system_category(), "Failed to create io_uring");
    }

    m_muxer = ring;
#endif
}

io_context_worker::~io_context_worker() {
    assert(!is_running());
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    CloseHandle(m_muxer);
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    io_uring *ring = static_cast<io_uring *>(std::malloc(sizeof(io_uring)));
    io_uring_queue_exit(ring);
    std::free(ring);
#endif
}

auto io_context_worker::run() noexcept -> void {
    if (m_is_running.exchange(true, std::memory_order_relaxed)) [[unlikely]]
        return;

    current_worker = this;

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    m_should_stop.store(false, std::memory_order_relaxed);
    m_thread_id.store(GetCurrentThreadId(), std::memory_order_relaxed);

    BOOL         result;
    DWORD        bytes;
    ULONG_PTR    key;
    LPOVERLAPPED ovlp;
    DWORD        error;

    std::vector<promise_base *> tasks;
    tasks.reserve(64);

    while (!m_should_stop.load(std::memory_order_relaxed)) [[likely]] {
        // Wait for 1 second.
        result = GetQueuedCompletionStatus(m_muxer, &bytes, &key, &ovlp, 1000);

        while (true) {
            if (result == FALSE) {
                error = GetLastError();
                if (error == WAIT_TIMEOUT)
                    break;
            } else {
                error = 0;
            }

            if (ovlp != nullptr) {
                auto *o = reinterpret_cast<overlapped *>(ovlp);

                o->error             = error;
                o->bytes_transferred = bytes;

                m_tasks.push_back(o->promise);
            }

            result = GetQueuedCompletionStatus(m_muxer, &bytes, &key, &ovlp, 0);
        }

        // Handle tasks.
        tasks.swap(m_tasks);
        for (const auto *task : tasks) {
            promise_base &stack_bottom = task->stack_bottom();
            task->coroutine().resume();
            if (stack_bottom.coroutine().done())
                stack_bottom.release();
        }

        tasks.clear();
    }

    m_thread_id.store(0, std::memory_order_relaxed);
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    m_should_stop.store(false, std::memory_order_relaxed);
    m_thread_id.store(gettid(), std::memory_order_relaxed);

    __kernel_timespec timeout{};

    io_uring     *ring = static_cast<io_uring *>(m_muxer);
    io_uring_cqe *cqe  = nullptr;

    std::vector<promise_base *> tasks;
    tasks.reserve(64);

    while (!m_should_stop.load(std::memory_order_relaxed)) [[likely]] {
        // Wait for 1 second.
        timeout.tv_sec  = 1;
        timeout.tv_nsec = 0;

        int result = io_uring_submit_and_wait_timeout(ring, &cqe, 1, &timeout, nullptr);
        while (result >= 0) {
            auto *ovlp = static_cast<overlapped *>(io_uring_cqe_get_data(cqe));

            if (ovlp != nullptr) {
                ovlp->flags  = cqe->flags;
                ovlp->result = cqe->res;
                m_tasks.push_back(ovlp->promise);
            }

            io_uring_cqe_seen(ring, cqe);
            result = io_uring_peek_cqe(ring, &cqe);
        }

        // Handle tasks.
        tasks.swap(m_tasks);
        for (const auto *task : tasks) {
            promise_base &stack_bottom = task->stack_bottom();
            task->coroutine().resume();
            if (stack_bottom.coroutine().done())
                stack_bottom.release();
        }

        tasks.clear();
    }

    m_thread_id.store(0, std::memory_order_relaxed);
#endif

    current_worker = nullptr;
    m_is_running.store(false, std::memory_order_relaxed);
}

auto io_context_worker::current() noexcept -> io_context_worker * {
    return current_worker;
}

auto io_context_worker::schedule(promise_base *promise) noexcept -> void {
    m_tasks.push_back(promise);

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    PostQueuedCompletionStatus(m_muxer, 0, 0, nullptr);
#elif defined(__linux) || defined(__linux__) || defined(__gnu_linux__)
    io_uring     *ring = static_cast<io_uring *>(m_muxer);
    io_uring_sqe *sqe  = io_uring_get_sqe(ring);

    if (sqe != nullptr) [[likely]] {
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, nullptr);
        io_uring_submit(ring);
    }
#endif
}

io_context::io_context()
    : m_is_running(),
      m_worker_count(std::max<std::size_t>(1, std::thread::hardware_concurrency())),
      m_workers(std::make_unique<io_context_worker[]>(m_worker_count)) {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) [[unlikely]]
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "Failed to start WinSock");
#endif
}

io_context::io_context(std::size_t count)
    : m_is_running(),
      m_worker_count(count ? count : std::max<std::size_t>(1, std::thread::hardware_concurrency())),
      m_workers(std::make_unique<io_context_worker[]>(m_worker_count)) {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) [[unlikely]]
        throw std::system_error(WSAGetLastError(), std::system_category(),
                                "Failed to start WinSock");
#endif
}

io_context::~io_context() {
    assert(!is_running());
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    WSACleanup();
#endif
}

auto io_context::run() noexcept -> void {
    if (m_is_running.exchange(true, std::memory_order_relaxed)) [[unlikely]]
        return;

    std::vector<std::thread> threads;
    threads.reserve(worker_count());

    for (std::size_t i = 0; i < worker_count(); ++i)
        threads.emplace_back([this, i]() { m_workers[i].run(); });

    for (auto &thread : threads)
        thread.join();

    m_is_running.store(false, std::memory_order_relaxed);
}

auto io_context::stop() noexcept -> void {
    for (std::size_t i = 0; i < worker_count(); ++i)
        m_workers[i].stop();
}
