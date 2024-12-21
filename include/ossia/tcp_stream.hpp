#pragma once

#include "inet_address.hpp"
#include "io_context.hpp"

#include <chrono>
#include <expected>
#include <system_error>

namespace ossia {

/// \class tcp_stream
/// \brief
///   \c tcp_stream is a class that represents a TCP connection. This class could only be used in
///   workers.
class tcp_stream {
public:
    /// \class connect_awaitable
    /// \brief
    ///   Awaitable object for connecting to a TCP server.
    class connect_awaitable {
    public:
        /// \brief
        ///   Create a new \c connect_awaitable object for asynchronous connect operation.
        /// \param[in] stream
        ///   The \c tcp_stream object to establish connection. The
        connect_awaitable(tcp_stream &stream, const inet_address &address) noexcept
            : m_ovlp(),
              m_socket(),
              m_address(&address),
              m_stream(&stream) {}

        /// \brief
        ///   C++20 coroutine API method. Always execute \c await_suspend().
        /// \return
        ///   This function always returns \c false.
        static constexpr auto await_ready() noexcept -> bool {
            return false;
        }

        /// \brief
        ///   Prepare for async connect operation and suspend the coroutine.
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
        ///   Get the result of the asynchronous connect operation.
        /// \return
        ///   Error code of the asynchronous connect operation. The error code is 0 if success.
        OSSIA_API auto await_resume() const noexcept -> std::error_code;

    private:
        /// \brief
        ///   Prepare for asynchronous connect operation and suspend this coroutine.
        OSSIA_API auto await_suspend() noexcept -> bool;

    private:
        detail::overlapped  m_ovlp;
        std::uintptr_t      m_socket;
        const inet_address *m_address;
        tcp_stream         *m_stream;
    };

    /// \class send_awaitable
    /// \brief
    ///   Awaitable object for sending data to a TCP endpoint.
    class send_awaitable {
    public:
        /// \brief
        ///   Create a new \c send_awaitable object for asynchronous send operation.
        /// \param socket
        ///   The socket handle to send data.
        /// \param data
        ///   Pointer to start of data to send.
        /// \param size
        ///   Size in byte of data to send.
        send_awaitable(std::uintptr_t socket, const void *data, std::uint32_t size) noexcept
            : m_ovlp(),
              m_socket(socket),
              m_data(data),
              m_size(size) {}

        /// \brief
        ///   C++20 coroutine API method. Always execute \c await_suspend().
        /// \return
        ///   This function always returns \c false.
        static constexpr auto await_ready() noexcept -> bool {
            return false;
        }

        /// \brief
        ///   Prepare for async send operation and suspend the coroutine.
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
        ///   Get the result of the asynchronous send operation.
        /// \return
        ///   Number of bytes sent if succeeded. Otherwise, return a system error code that
        ///   represents the IO error.
        OSSIA_API auto await_resume() const noexcept
            -> std::expected<std::uint32_t, std::error_code>;

    private:
        /// \brief
        ///   Prepare for asynchronous send operation and suspend this coroutine.
        OSSIA_API auto await_suspend() noexcept -> bool;

    private:
        detail::overlapped m_ovlp;
        std::uintptr_t     m_socket;
        const void        *m_data;
        std::uint32_t      m_size;
    };

    /// \class receive_awaitable
    /// \brief
    ///   Awaitable object for receiving data from a TCP endpoint.
    class receive_awaitable {
    public:
        /// \brief
        ///   Create a new \c receive_awaitable object for asynchronous receive operation.
        /// \param socket
        ///   The socket handle to receive data.
        /// \param[in] data
        ///   Pointer to start of buffer to receive data.
        /// \param size
        ///   Size in byte of buffer to store the received data.
        receive_awaitable(std::uintptr_t socket, void *data, std::uint32_t size) noexcept
            : m_ovlp(),
              m_socket(socket),
              m_data(data),
              m_size(size) {}

        /// \brief
        ///   C++20 coroutine API method. Always execute \c await_suspend().
        /// \return
        ///   This function always returns \c false.
        static constexpr auto await_ready() noexcept -> bool {
            return false;
        }

        /// \brief
        ///   Prepare for async receive operation and suspend the coroutine.
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
        ///   Get the result of the asynchronous receive operation.
        /// \return
        ///   Number of bytes received if succeeded. Otherwise, return a system error code that
        ///   represents the IO error.
        OSSIA_API auto await_resume() const noexcept
            -> std::expected<std::uint32_t, std::error_code>;

    private:
        /// \brief
        ///   Prepare for asynchronous receive operation and suspend this coroutine.
        OSSIA_API auto await_suspend() noexcept -> bool;

    private:
        detail::overlapped m_ovlp;
        std::uintptr_t     m_socket;
        void              *m_data;
        std::uint32_t      m_size;
    };

public:
    /// \brief
    ///   Create an empty \c tcp_stream object. Empty \c tcp_stream object is not connected to any
    ///   TCP endpoint.
    OSSIA_API tcp_stream() noexcept;

    /// \brief
    ///   For internal usage. Create a new \c tcp_stream object with a socket handle and address.
    /// \param socket
    ///   The socket handle of the TCP connection.
    /// \param address
    ///   The peer address of the TCP connection.
    tcp_stream(std::uintptr_t socket, const inet_address &address) noexcept
        : m_socket(socket),
          m_address(address) {}

    /// \brief
    ///   \c tcp_stream is not copyable.
    tcp_stream(const tcp_stream &other) = delete;

    /// \brief
    ///   Move constructor of \c tcp_stream object.
    /// \param[in, out] other
    ///   The \c tcp_stream object to move. The moved \c tcp_stream object will be empty.
    OSSIA_API tcp_stream(tcp_stream &&other) noexcept;

    /// \brief
    ///   Destroy thie TCP connection and release all resources.
    OSSIA_API ~tcp_stream();

    /// \brief
    ///   \c tcp_stream is not copyable.
    auto operator=(const tcp_stream &other) = delete;

    /// \brief
    ///   Move assignment operator of \c tcp_stream object.
    /// \param[in, out] other
    ///   The \c tcp_stream object to move. The moved \c tcp_stream object will be empty.
    ///   Self-assignment is handled but not recommended.
    /// \return
    ///   Reference to this \c tcp_stream object.
    OSSIA_API auto operator=(tcp_stream &&other) noexcept -> tcp_stream &;

    /// \brief
    ///   Get peer address of the TCP connection. It is undefined behavior to get peer address from
    ///   an empty \c tcp_stream object.
    /// \return
    ///   Peer address of this TCP connection.
    [[nodiscard]]
    auto peer_address() const noexcept -> const inet_address & {
        return m_address;
    }

    /// \brief
    ///   Connect to the specified peer address. This method will block current thread until the
    ///   connection is established or any error occurs.
    /// \remarks
    ///   This method does not affect this \c tcp_stream object if failed to establish new
    ///   connection.
    /// \param address
    ///   The peer address to connect.
    /// \return
    ///   A system error code that indicates the result of the connection operation. The error code
    ///   is 0 if success.
    OSSIA_API auto connect(const inet_address &address) noexcept -> std::error_code;

    /// \brief
    ///   Connect to the specified peer address asynchronously. This method will suspend this
    ///   coroutine until the connection is established or any error occurs.
    /// \remarks
    ///   This method does not affect this \c tcp_stream object if failed to establish new
    ///   connection.
    /// \param address
    ///   The peer address to connect.
    /// \return
    ///   A system error code that indicates the result of the connection operation. The error code
    ///   is 0 if success.
    [[nodiscard]]
    auto connect_async(const inet_address &address) noexcept -> connect_awaitable {
        return connect_awaitable(*this, address);
    }

    /// \brief
    ///   Send data to the peer TCP endpoint. This method will block current thread until the data
    ///   is sent or any error occurs.
    /// \param data
    ///   Pointer to start of data to send.
    /// \param size
    ///   Size in byte of data to send.
    /// \return
    ///   Number of bytes sent if succeeded. Otherwise, return a system error code that represents
    ///   the IO error.
    OSSIA_API auto send(const void *data, std::uint32_t size) noexcept
        -> std::expected<std::uint32_t, std::error_code>;

    /// \brief
    ///   Send data to the peer TCP endpoint asynchronously. This method will suspend this coroutine
    ///   until the data is sent or any error occurs.
    /// \param data
    ///   Pointer to start of data to send.
    /// \param size
    ///   Size in byte of data to send.
    /// \return
    ///   Number of bytes sent if succeeded. Otherwise, return a system error code that represents
    ///   the IO error.
    [[nodiscard]]
    auto send_async(const void *data, std::uint32_t size) noexcept -> send_awaitable {
        return send_awaitable(m_socket, data, size);
    }

    /// \brief
    ///   Receive data from the peer TCP endpoint. This method will block current thread until the
    ///   data is received or any error occurs.
    /// \param[out] data
    ///   Pointer to start of buffer to receive data.
    /// \param size
    ///   Size in byte of buffer to store the received data.
    /// \return
    ///   Number of bytes received if succeeded. Otherwise, return a system error code that
    ///   represents the IO error.
    OSSIA_API auto receive(void *data, std::uint32_t size) noexcept
        -> std::expected<std::uint32_t, std::error_code>;

    /// \brief
    ///   Receive data from the peer TCP endpoint asynchronously. This method will suspend this
    ///   coroutine until the data is received or any error occurs.
    /// \param[out] data
    ///   Pointer to start of buffer to receive data.
    /// \param size
    ///   Size in byte of buffer to store the received data.
    /// \return
    ///   Number of bytes received if succeeded. Otherwise, return a system error code that
    ///   represents the IO error.
    [[nodiscard]]
    auto receive_async(void *data, std::uint32_t size) noexcept -> receive_awaitable {
        return receive_awaitable(m_socket, data, size);
    }

    /// \brief
    ///   Enable or disable keep-alive mechanism of this TCP connection.
    /// \param enable
    ///   \c true to enable keep-alive mechanism. \c false to disable keep-alive mechanism.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    OSSIA_API auto set_keep_alive(bool enable) noexcept -> std::error_code;

    /// \brief
    ///   Enable or disable TCP no-delay mechanism of this TCP connection.
    /// \param enable
    ///   \c true to enable TCP no-delay mechanism. \c false to disable TCP no-delay mechanism.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    OSSIA_API auto set_no_delay(bool enable) noexcept -> std::error_code;

    /// \brief
    ///   Set send timeout of this TCP connection.
    /// \tparam Rep
    ///   Type of the duration representation.
    /// \tparam Duration
    ///   Type of the duration.
    /// \param timeout
    ///   Timeout duration. Use 0 or negative value for never timeout.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    template <class Rep, class Duration>
    auto set_send_timeout(std::chrono::duration<Rep, Duration> timeout) noexcept
        -> std::error_code {
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();
        milliseconds      = milliseconds < 0 ? 0 : milliseconds;
        return this->set_send_timeout(static_cast<std::uint32_t>(milliseconds));
    }

    /// \brief
    ///   Set receive timeout of this TCP connection.
    /// \tparam Rep
    ///   Type of the duration representation.
    /// \tparam Duration
    ///   Type of the duration.
    /// \param timeout
    ///   Timeout duration. Use 0 or negative value for never timeout.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    template <class Rep, class Duration>
    auto set_receive_timeout(std::chrono::duration<Rep, Duration> timeout) noexcept
        -> std::error_code {
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();
        milliseconds      = milliseconds < 0 ? 0 : milliseconds;
        return this->set_receive_timeout(static_cast<std::uint32_t>(milliseconds));
    }

    /// \brief
    ///   Close this TCP connection and release all resources. Closing a \c tcp_stream object will
    ///   cause errors for pending IO operations. This method does nothing if this is an empty
    ///   \c tcp_stream object.
    OSSIA_API auto close() noexcept -> void;

private:
    /// \brief
    ///   Set send timeout of this TCP connection.
    /// \param timeout
    ///   Timeout in milliseconds. Use 0 for never timeout.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    OSSIA_API auto set_send_timeout(std::uint32_t timeout) noexcept -> std::error_code;

    /// \brief
    ///   Set receive timeout of this TCP connection.
    /// \param timeout
    ///   Timeout in milliseconds. Use 0 for never timeout.
    /// \return
    ///   A system error code that indicates the result of the operation. The error code is 0 if
    ///   success.
    OSSIA_API auto set_receive_timeout(std::uint32_t timeout) noexcept -> std::error_code;

private:
    std::uintptr_t m_socket;
    inet_address   m_address;
};

} // namespace ossia
