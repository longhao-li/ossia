#include "ossia/tcp_server.hpp"

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

auto tcp_server::accept_awaitable::await_resume() const noexcept
    -> std::expected<tcp_stream, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_ovlp.error != 0) [[unlikely]] {
        if (m_socket != invalid_socket)
            closesocket(static_cast<SOCKET>(m_socket));

        return std::unexpected(
            std::error_code(static_cast<int>(m_ovlp.error), std::system_category()));
    }

    return tcp_stream(m_socket, m_address);
#endif
}

auto tcp_server::accept_awaitable::await_suspend() noexcept -> bool {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    // Create a new socket for the incoming connection.
    auto *addr = reinterpret_cast<sockaddr *>(&m_address);
    m_socket   = WSASocketW(addr->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                            WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);

    if (m_socket == invalid_socket) [[unlikely]] {
        m_ovlp.error = WSAGetLastError();
        return false;
    }

    // Register the socket to IOCP.
    auto *worker = io_context_worker::current();
    assert(worker != nullptr);

    if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_socket), worker->muxer(), 0, 0) ==
        nullptr) [[unlikely]] {
        m_ovlp.error = GetLastError();
        return false;
    }

    // Disable IOCP notification if IO event is handled immediately.
    if (SetFileCompletionNotificationModes(reinterpret_cast<HANDLE>(m_socket),
                                           FILE_SKIP_SET_EVENT_ON_HANDLE |
                                               FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) == FALSE)
        [[unlikely]] {
        m_ovlp.error = GetLastError();
        return false;
    }

    // Acquire AcceptEx function pointer.
    LPFN_ACCEPTEX accept_ex = reinterpret_cast<LPFN_ACCEPTEX>(m_server->m_accept_ex);
    assert(accept_ex != nullptr);

    // Try to accept a new incoming connection.
    // FIXME: Is it safe to make bytes a temporary variable?
    DWORD bytes = 0;
    if (accept_ex(m_server->m_socket, m_socket, &m_address, 0, 0, sizeof(m_address), &bytes,
                  reinterpret_cast<LPOVERLAPPED>(&m_ovlp)) == TRUE) {
        m_ovlp.error = 0;
        return false;
    }

    DWORD error = WSAGetLastError();
    if (error == ERROR_IO_PENDING) [[likely]]
        return true;

    m_ovlp.error = error;
    return false;
#endif
}

tcp_server::tcp_server() noexcept : m_socket(invalid_socket), m_address() {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    m_accept_ex = nullptr;
#endif
}

tcp_server::tcp_server(tcp_server &&other) noexcept
    : m_socket(other.m_socket),
      m_address(other.m_address) {
    other.m_socket = invalid_socket;

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    m_accept_ex       = other.m_accept_ex;
    other.m_accept_ex = nullptr;
#endif
}

tcp_server::~tcp_server() {
    close();
}

auto tcp_server::operator=(tcp_server &&other) noexcept -> tcp_server & {
    if (this == &other) [[unlikely]]
        return *this;

    close();

    m_socket  = other.m_socket;
    m_address = other.m_address;

    other.m_socket = invalid_socket;

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    m_accept_ex       = other.m_accept_ex;
    other.m_accept_ex = nullptr;
#endif

    return *this;
}

auto tcp_server::bind(const inet_address &address) noexcept -> std::error_code {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    // Create a new socket for the server.
    auto  *addr = reinterpret_cast<const sockaddr *>(&address);
    SOCKET s    = WSASocketW(addr->sa_family, SOCK_STREAM, IPPROTO_TCP, nullptr, 0,
                             WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);

    if (s == invalid_socket) [[unlikely]]
        return std::error_code(WSAGetLastError(), std::system_category());

    { // Enable SO_REUSEADDR option.
        DWORD value = TRUE;
        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&value),
                       sizeof(value)) == SOCKET_ERROR) {
            DWORD error = WSAGetLastError();
            closesocket(s);
            return std::error_code(static_cast<int>(error), std::system_category());
        }
    }

    // Register the socket to IOCP.
    auto *worker = io_context_worker::current();
    assert(worker != nullptr);
    if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), worker->muxer(), 0, 0) == nullptr)
        [[unlikely]] {
        DWORD error = GetLastError();
        closesocket(s);
        return std::error_code(static_cast<int>(error), std::system_category());
    }

    // Disable IOCP notification if IO event is handled immediately.
    if (SetFileCompletionNotificationModes(reinterpret_cast<HANDLE>(s),
                                           FILE_SKIP_SET_EVENT_ON_HANDLE |
                                               FILE_SKIP_COMPLETION_PORT_ON_SUCCESS) == FALSE)
        [[unlikely]] {
        DWORD error = GetLastError();
        closesocket(s);
        return std::error_code(static_cast<int>(error), std::system_category());
    }

    // Bind the socket to the specified address.
    if (::bind(s, addr, addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) ==
        SOCKET_ERROR) [[unlikely]] {
        DWORD error = WSAGetLastError();
        closesocket(s);
        return std::error_code(static_cast<int>(error), std::system_category());
    }

    // Start listening on the socket.
    if (listen(s, SOMAXCONN) == SOCKET_ERROR) [[unlikely]] {
        DWORD error = WSAGetLastError();
        closesocket(s);
        return std::error_code(static_cast<int>(error), std::system_category());
    }

    // Acquire AcceptEx function pointer.
    LPFN_ACCEPTEX accept_ex = nullptr;

    {
        GUID  guid  = WSAID_ACCEPTEX;
        DWORD bytes = 0;
        if (WSAIoctl(s, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &accept_ex,
                     sizeof(accept_ex), &bytes, nullptr, nullptr) == SOCKET_ERROR) [[unlikely]] {
            DWORD error = WSAGetLastError();
            closesocket(s);
            return std::error_code(static_cast<int>(error), std::system_category());
        }
    }

    close();

    m_socket    = s;
    m_address   = address;
    m_accept_ex = reinterpret_cast<void *>(accept_ex);

    return std::error_code();
#endif
}

auto tcp_server::accept() const noexcept -> std::expected<tcp_stream, std::error_code> {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    inet_address address;
    int          addrlen = sizeof(address);

    SOCKET s = WSAAccept(m_socket, reinterpret_cast<sockaddr *>(&address), &addrlen, nullptr, 0);
    if (s == invalid_socket) [[unlikely]]
        return std::unexpected(std::error_code(WSAGetLastError(), std::system_category()));

    return tcp_stream(s, address);
#endif
}

auto tcp_server::close() noexcept -> void {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    if (m_socket != invalid_socket) {
        closesocket(static_cast<SOCKET>(m_socket));
        m_socket = invalid_socket;
    }
#endif
}
