#pragma once

#include "future.hpp"

#include <atomic>
#include <vector>

namespace ossia::detail {

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
/// \struct overlapped
/// \brief
///   Overlapped structure for Windows \c IOCP.
struct overlapped {
    std::uintptr_t internal;
    std::uintptr_t internal_high;
    std::uint32_t  offset;
    std::uint32_t  offset_high;
    void          *event;
    std::uint32_t  error;
    std::uint32_t  bytes_transferred;
    promise_base  *promise;
};
#elif defined(__linux__) || defined(__linux)
/// \struct overlapped
/// \brief
///   Overlapped structure for Linux \c io_uring.
struct overlapped {
    std::int32_t  flags;
    std::int32_t  result;
    promise_base *promise;
};
#endif

/// \class io_context_worker
/// \brief
///   Worker class for IO context.
class io_context_worker {
public:
    /// \brief
    ///   Create a new worker and initialize the IO muxer. \c IOCP is used for Windows and
    ///   \c io_uring is used for Linux.
    /// \throws std::system_error
    ///   Thrown if failed to initialize the IO muxer.
    OSSIA_API io_context_worker();

    /// \brief
    ///   \c io_context_worker is not copyable.
    io_context_worker(const io_context_worker &other) = delete;

    /// \brief
    ///   \c io_context_worker is not movable.
    io_context_worker(io_context_worker &&other) = delete;

    /// \brief
    ///   Destroy this worker and the IO muxer. This worker must be stopped before destruction.
    OSSIA_API ~io_context_worker();

    /// \brief
    ///   \c io_context_worker is not copyable.
    auto operator=(const io_context_worker &other) = delete;

    /// \brief
    ///   \c io_context_worker is not movable.
    auto operator=(io_context_worker &&other) = delete;

    /// \brief
    ///   Check if this worker is running.
    /// \retval true
    ///   This worker is running.
    /// \retval false
    ///   This worker is not running.
    [[nodiscard]]
    auto is_running() const noexcept -> bool {
        return m_is_running.load(std::memory_order_relaxed);
    }

    /// \brief
    ///   Get thread ID of this worker.
    /// \return
    ///   Thread ID of this worker. The return value is valid only if this worker is running.
    [[nodiscard]]
    auto thread_id() const noexcept -> std::size_t {
        return m_thread_id.load(std::memory_order_relaxed);
    }

    /// \brief
    ///   Start this worker and handle IO requests. This method will block current thread. It is
    ///   safe to call this method for multiple-times in different threads, but only one will start
    ///   running.
    OSSIA_API auto run() noexcept -> void;

    /// \brief
    ///   Request this worker to stop. This method only sets the stop flag and does not block. It
    ///   may take some time to stop the worker.
    auto stop() noexcept -> void {
        m_should_stop.store(true, std::memory_order_relaxed);
    }

    /// \brief
    ///   Schedule a new task in this worker. This method is not concurrent safe. The scheduled task
    ///   will be executed as soon as possible.
    /// \tparam T
    ///   Return type of the scheduled task. Usually this should be \c void.
    /// \param task
    ///   The task to be scheduled. This task should be the coroutine stack bottom task.
    template <class T>
    auto schedule(future<T> task) noexcept -> void {
        auto coroutine = task.detach();
        this->schedule(&coroutine.promise());
    }

    /// \brief
    ///   For internal usage. Get the IO muxer handle.
    /// \return
    ///   IO muxer handle. For Windows, this is the \c HANDLE of \c IOCP. For Linux, this is pointer
    ///   to the liburing \c io_uring object.
    [[nodiscard]]
    auto muxer() const noexcept -> void * {
        return m_muxer;
    }

private:
    /// \brief
    ///   For internal usage. Schedule a task to be executed in this worker. This method is not
    ///   concurrent safe. The scheduled task will be executed as soon as possible.
    /// \param[in] promise
    ///   Promise of the task to be executed. This method will take over the ownership of the
    ///   promise if this promise is the stack bottom.
    OSSIA_API auto schedule(promise_base *promise) noexcept -> void;

private:
    /// \brief
    ///   Running flag for this worker.
    std::atomic_bool m_is_running;

    /// \brief
    ///   Thread ID of this worker. This value is valid only when the worker is running.
    std::atomic_size_t m_thread_id;

    /// \brief
    ///   IO muxer handle. For Windows, this is the \c HANDLE of \c IOCP. For Linux, this is pointer
    ///   to the liburing \c io_uring object.
    void *m_muxer;

    /// \brief
    ///   Task queue for this worker.
    std::vector<promise_base *> m_tasks;

    /// \brief
    ///   Stop flag for this worker. This value is aligned up with cacheline size to avoid cacheline
    ///   lock on atomic operation as possible.
    alignas(64) std::atomic_bool m_should_stop;
};

} // namespace ossia::detail
