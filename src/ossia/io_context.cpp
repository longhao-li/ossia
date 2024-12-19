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
#endif
}

io_context_worker::~io_context_worker() {
    assert(!is_running());
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    CloseHandle(m_muxer);
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
