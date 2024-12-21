#pragma once

#include "tcp_stream.hpp"

namespace ossia {

/// \class tcp_server
/// \brief
///   \c tcp_server is a class that represents a TCP server. This class could only be used in
///   workers.
class tcp_server {
public:
    /// \class accept_awaitable
    /// \brief
    ///   Awaitable object for accepting a new TCP connection.
    class accept_awaitable {
    public:
        /// \brief
        ///   Create a new \c accept_awaitable object for asynchronous accept operation.
        /// \param[in] server
        ///   The \c tcp_server object to accept new connection.
        accept_awaitable(tcp_server &server) noexcept
            : m_ovlp(),
              m_server(&server),
              m_socket(),
              m_address(),
              m_padding{} {}

        /// \brief
        ///   C++20 coroutine API method. Always execute \c await_suspend().
        /// \return
        ///   This function always returns \c false.
        static constexpr auto await_ready() noexcept -> bool {
            return false;
        }

        /// \brief
        ///   Prepare for async accept operation and suspend the coroutine.
        /// \tparam T
        ///   Type of promise of current coroutine.
        /// \param coroutine
        ///   Current coroutine handle.
        /// \retval true
        ///   This coroutine should be suspended and resumed later.
        /// \retval false
        ///   This coroutine should not be suspended and should be resumed immediately.
        template <class T>
        auto await_suspend(std::coroutine_handle<T> coroutine) noexcept -> bool {
            m_ovlp.promise = &static_cast<detail::promise_base &>(coroutine.promise());
            return this->await_suspend();
        }

        /// \brief
        ///   Get the result of the asynchronous accept operation.
        /// \return
        ///   A new \c tcp_stream object if succeeded. Otherwise, return a system error code that
        ///   represents system IO error.
        OSSIA_API auto await_resume() const noexcept -> std::expected<tcp_stream, std::error_code>;

    private:
        /// \brief
        ///   Prepare for asynchronous accept operation and suspend this coroutine.
        OSSIA_API auto await_suspend() noexcept -> bool;

    private:
        detail::overlapped m_ovlp;
        const tcp_server  *m_server;
        std::uintptr_t     m_socket;
        inet_address       m_address;
        char               m_padding[16];
    };

public:
    /// \brief
    ///   Create a new \c tcp_server object. Empty server object is not valid for use before
    ///   binding.
    OSSIA_API tcp_server() noexcept;

    /// \brief
    ///   \c tcp_server is not copyable.
    tcp_server(const tcp_server &other) = delete;

    /// \brief
    ///   Move constructor of \c tcp_server object.
    /// \param[in, out] other
    ///   The \c tcp_server object to move. The moved \c tcp_server object will be empty.
    OSSIA_API tcp_server(tcp_server &&other) noexcept;

    /// \brief
    ///   Stop listening and release all resources.
    OSSIA_API ~tcp_server();

    /// \brief
    ///   \c tcp_server is not copyable.
    auto operator=(const tcp_server &other) = delete;

    /// \brief
    ///   Move assignment operator of \c tcp_server object.
    /// \param[in, out] other
    ///   The \c tcp_server object to move. The moved \c tcp_server object will be empty.
    ///   Self-assignment is handled but not recommended.
    /// \return
    ///   Reference to this \c tcp_server object.
    OSSIA_API auto operator=(tcp_server &&other) noexcept -> tcp_server &;

    /// \brief
    ///   Get local address of this server. It is undefined behavior to get local address of an
    ///   empty server.
    /// \return
    ///   Local address of this server.
    [[nodiscard]]
    auto local_address() const noexcept -> const inet_address & {
        return m_address;
    }

    /// \brief
    ///   Start listening on the specified address.
    /// \param[in] address
    ///   The address to bind. The address could be either an IPv4 or IPv6 address.
    /// \return
    ///   An \c std::error_code object that represents system error. The error code is 0 if this
    ///   operation is succeeded.
    OSSIA_API auto bind(const inet_address &address) noexcept -> std::error_code;

    /// \brief
    ///   Accept a new incoming TCP connection. This method will block current thread until a new
    ///   incoming connection is established or any error occurs.
    /// \return
    ///   A new \c tcp_stream object if succeeded. Otherwise, return a system error code that
    ///   represents system IO error.
    OSSIA_API auto accept() const noexcept -> std::expected<tcp_stream, std::error_code>;

    /// \brief
    ///   Accept a new incoming TCP connection asynchronously. This method will suspend this
    ///   coroutine until a new incoming connection is established or any error occurs.
    /// \return
    ///   A new \c tcp_stream object if succeeded. Otherwise, return a system error code that
    ///   represents system IO error.
    [[nodiscard]]
    auto accept_async() noexcept -> accept_awaitable {
        return accept_awaitable(*this);
    }

    /// \brief
    ///   Stop listening and release all resources. Closing a \c tcp_server object will cause errors
    ///   for pending accept operations. This method does nothing if this is an empty \c tcp_server
    ///   object.
    OSSIA_API auto close() noexcept -> void;

private:
    std::uintptr_t m_socket;
    inet_address   m_address;

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    void *m_accept_ex;
#endif
};

} // namespace ossia
