#include "ossia/tcp_stream.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#    include <WS2tcpip.h>
#    include <WinSock2.h>
#    include <mswsock.h>
#endif

#include <cassert>

using namespace ossia;
using namespace ossia::detail;

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
inline constexpr std::uintptr_t invalid_socket = INVALID_SOCKET;
#else
inline constexpr std::uintptr_t invalid_socket = static_cast<std::uintptr_t>(-1);
#endif

auto tcp_stream::connect_awaitable::await_resume() const noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_ovlp.error == 0) {
        if (m_stream->m_socket != invalid_socket)
            closesocket(static_cast<SOCKET>(m_stream->m_socket));

        m_stream->m_socket  = m_socket;
        m_stream->m_address = *m_address;

        return std::error_code();
    }

    if (m_socket != invalid_socket)
        closesocket(static_cast<SOCKET>(m_socket));

    return std::error_code(static_cast<int>(m_ovlp.error), std::system_category());
#endif
}

auto tcp_stream::connect_awaitable::await_suspend() noexcept -> bool {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    auto  *addr = reinterpret_cast<const sockaddr *>(m_address);
    SOCKET s    = WSASocketW(addr->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                             WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);

    if (s == invalid_socket) [[unlikely]] {
        m_ovlp.error = WSAGetLastError();
        return false;
    }

    m_socket = s;

    // ConnectEx requires manually binding.
    if (addr->sa_family == AF_INET) {
        sockaddr_in local{
            .sin_family = AF_INET,
            .sin_port   = 0,
            .sin_addr   = INADDR_ANY,
            .sin_zero   = {},
        };

        if (bind(s, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == SOCKET_ERROR)
            [[unlikely]] {
            m_ovlp.error = WSAGetLastError();
            return false;
        }
    } else {
        sockaddr_in6 local{
            .sin6_family   = AF_INET6,
            .sin6_port     = 0,
            .sin6_flowinfo = 0,
            .sin6_addr     = in6addr_any,
            .sin6_scope_id = 0,
        };

        if (bind(s, reinterpret_cast<sockaddr *>(&local), sizeof(local)) == SOCKET_ERROR)
            [[unlikely]] {
            m_ovlp.error = WSAGetLastError();
            return false;
        }
    }

    { // Register to IOCP.
        auto *worker = io_context_worker::current();
        assert(worker != nullptr);
        if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), worker->muxer(), 0, 0) == nullptr)
            [[unlikely]] {
            m_ovlp.error = GetLastError();
            return false;
        }
    }

    // Disable IOCP notification once IO is handled immediately.
    if (SetFileCompletionNotificationModes(reinterpret_cast<HANDLE>(s),
                                           FILE_SKIP_SET_EVENT_ON_HANDLE |
                                               FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) == FALSE)
        [[unlikely]] {
        m_ovlp.error = GetLastError();
        return false;
    }

    // Acquire ConnectEx function pointer.
    LPFN_CONNECTEX connect_ex = nullptr;

    {
        GUID  guid  = WSAID_CONNECTEX;
        DWORD bytes = 0;
        if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &connect_ex,
                     sizeof(connect_ex), &bytes, nullptr, nullptr) == SOCKET_ERROR) [[unlikely]] {
            m_ovlp.error = WSAGetLastError();
            return false;
        }
    }

    { // Try to connect to the peer address.
        int   addrlen = addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
        DWORD bytes   = 0;

        // Connection established immediately. Unlikely to happen.
        if (connect_ex(s, addr, addrlen, nullptr, 0, &bytes,
                       reinterpret_cast<LPOVERLAPPED>(&m_ovlp)) == TRUE) [[unlikely]] {
            m_ovlp.error = 0;
            return false;
        }
    }

    DWORD error = WSAGetLastError();
    if (error == ERROR_IO_PENDING) [[likely]]
        return true;

    m_ovlp.error = error;
    return false;
#endif
}

auto tcp_stream::send_awaitable::await_resume() const noexcept
    -> std::expected<std::uint32_t, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_ovlp.error == 0) [[likely]]
        return m_ovlp.bytes_transferred;

    return std::unexpected(std::error_code(static_cast<int>(m_ovlp.error), std::system_category()));
#endif
}

auto tcp_stream::send_awaitable::await_suspend() noexcept -> bool {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD  bytes = 0;
    WSABUF buffer{
        .len = m_size,
        .buf = static_cast<char *>(const_cast<void *>(m_data)),
    };

    // Send returned immediately. Do not suspend this coroutine.
    if (WSASend(m_socket, &buffer, 1, &bytes, 0, reinterpret_cast<LPOVERLAPPED>(&m_ovlp),
                nullptr) == TRUE) [[unlikely]] {
        m_ovlp.error             = 0;
        m_ovlp.bytes_transferred = bytes;
        return false;
    }

    DWORD error = WSAGetLastError();

    if (error == 0) {
        m_ovlp.error             = 0;
        m_ovlp.bytes_transferred = bytes;
        return false;
    }

    if (error == WSA_IO_PENDING) [[likely]]
        return true;

    m_ovlp.error = error;
    return false;
#endif
}

auto tcp_stream::receive_awaitable::await_resume() const noexcept
    -> std::expected<std::uint32_t, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_ovlp.error == 0) [[likely]]
        return m_ovlp.bytes_transferred;

    return std::unexpected(std::error_code(static_cast<int>(m_ovlp.error), std::system_category()));
#endif
}

auto tcp_stream::receive_awaitable::await_suspend() noexcept -> bool {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD  bytes = 0;
    DWORD  flags = 0;
    WSABUF buffer{
        .len = m_size,
        .buf = static_cast<char *>(m_data),
    };

    // Receive returned immediately. Do not suspend this coroutine.
    if (WSARecv(m_socket, &buffer, 1, &bytes, &flags, reinterpret_cast<LPOVERLAPPED>(&m_ovlp),
                nullptr) == TRUE) [[unlikely]] {
        m_ovlp.error             = 0;
        m_ovlp.bytes_transferred = bytes;
        return false;
    }

    DWORD error = WSAGetLastError();

    if (error == 0) {
        m_ovlp.error             = 0;
        m_ovlp.bytes_transferred = bytes;
        return false;
    }

    if (error == WSA_IO_PENDING) [[likely]]
        return true;

    m_ovlp.error = error;
    return false;
#endif
}

tcp_stream::tcp_stream() noexcept : m_socket(invalid_socket), m_address() {}

tcp_stream::tcp_stream(tcp_stream &&other) noexcept
    : m_socket(other.m_socket),
      m_address(other.m_address) {
    other.m_socket = invalid_socket;
}

tcp_stream::~tcp_stream() {
    close();
}

auto tcp_stream::operator=(tcp_stream &&other) noexcept -> tcp_stream & {
    if (this == &other) [[unlikely]]
        return *this;

    close();

    m_socket  = other.m_socket;
    m_address = other.m_address;

    other.m_socket = invalid_socket;
    return *this;
}

auto tcp_stream::connect(const inet_address &address) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    auto  *addr = reinterpret_cast<sockaddr *>(&m_address);
    SOCKET s    = WSASocketW(addr->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                             WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);

    if (s == invalid_socket) [[unlikely]]
        return std::error_code(WSAGetLastError(), std::system_category());

    { // Register to IOCP.
        auto *worker = io_context_worker::current();
        assert(worker != nullptr);
        if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), worker->muxer(), 0, 0) == nullptr)
            [[unlikely]] {
            DWORD error = GetLastError();
            closesocket(s);
            return std::error_code(static_cast<int>(error), std::system_category());
        }
    }

    // Disable IOCP notification once IO is handled immediately.
    if (SetFileCompletionNotificationModes(reinterpret_cast<HANDLE>(s),
                                           FILE_SKIP_SET_EVENT_ON_HANDLE |
                                               FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) == FALSE)
        [[unlikely]] {
        DWORD error = GetLastError();
        closesocket(s);
        return std::error_code(static_cast<int>(error), std::system_category());
    }

    { // Try to connect to the peer address.
        int addrlen = addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        // Connection established immediately. Unlikely to happen.
        if (WSAConnect(s, addr, addrlen, nullptr, nullptr, nullptr, nullptr) == FALSE)
            [[unlikely]] {
            DWORD error = WSAGetLastError();
            closesocket(s);
            return std::error_code(static_cast<int>(error), std::system_category());
        }
    }

    close();

    m_socket  = s;
    m_address = address;

    return std::error_code();
#endif
}

auto tcp_stream::send(const void *data, std::uint32_t size) noexcept
    -> std::expected<std::uint32_t, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD  bytes = 0;
    WSABUF buffer{
        .len = size,
        .buf = static_cast<char *>(const_cast<void *>(data)),
    };

    if (WSASend(static_cast<SOCKET>(m_socket), &buffer, 1, &bytes, 0, nullptr, nullptr) == TRUE)
        return bytes;

    DWORD error = WSAGetLastError();
    return std::unexpected(std::error_code(static_cast<int>(error), std::system_category()));
#endif
}

auto tcp_stream::receive(void *data, std::uint32_t size) noexcept
    -> std::expected<std::uint32_t, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD  bytes = 0;
    DWORD  flags = 0;
    WSABUF buffer{
        .len = size,
        .buf = static_cast<char *>(data),
    };

    if (WSARecv(static_cast<SOCKET>(m_socket), &buffer, 1, &bytes, &flags, nullptr, nullptr) ==
        TRUE)
        return bytes;

    DWORD error = WSAGetLastError();
    return std::unexpected(std::error_code(static_cast<int>(error), std::system_category()));
#endif
}

auto tcp_stream::set_keep_alive(bool enable) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD value = enable ? 1 : 0;
    if (setsockopt(static_cast<SOCKET>(m_socket), SOL_SOCKET, SO_KEEPALIVE,
                   reinterpret_cast<const char *>(&value), sizeof(value)) == SOCKET_ERROR)
        return std::error_code(WSAGetLastError(), std::system_category());

    return std::error_code();
#endif
}

auto tcp_stream::set_no_delay(bool enable) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD value = enable ? 1 : 0;
    if (setsockopt(static_cast<SOCKET>(m_socket), IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char *>(&value), sizeof(value)) == SOCKET_ERROR)
        return std::error_code(WSAGetLastError(), std::system_category());

    return std::error_code();
#endif
}

auto tcp_stream::close() noexcept -> void {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_socket != invalid_socket) {
        closesocket(static_cast<SOCKET>(m_socket));
        m_socket = invalid_socket;
    }
#else
    if (m_socket != invalid_socket) {
        ::close(static_cast<int>(m_socket));
        m_socket = invalid_socket;
    }
#endif
}

auto tcp_stream::set_send_timeout(std::uint32_t timeout) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD value = timeout;
    if (setsockopt(static_cast<SOCKET>(m_socket), SOL_SOCKET, SO_SNDTIMEO,
                   reinterpret_cast<const char *>(&value), sizeof(value)) == SOCKET_ERROR)
        return std::error_code(WSAGetLastError(), std::system_category());

    return std::error_code();
#endif
}

auto tcp_stream::set_receive_timeout(std::uint32_t timeout) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    DWORD value = timeout;
    if (setsockopt(static_cast<SOCKET>(m_socket), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char *>(&value), sizeof(value)) == SOCKET_ERROR)
        return std::error_code(WSAGetLastError(), std::system_category());

    return std::error_code();
#endif
}
