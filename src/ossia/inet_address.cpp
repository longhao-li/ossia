#include "ossia/inet_address.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#    include <WS2tcpip.h>
#    include <WinSock2.h>
#endif

#include <cstring>
#include <stdexcept>

using namespace ossia;

ip_address::ip_address(std::string_view address) : m_is_v6(), m_addr() {
    char buffer[INET6_ADDRSTRLEN];
    if (address.size() >= std::size(buffer)) [[unlikely]]
        throw std::invalid_argument("Invalid IP address: " + std::string(address));

    // Copy the address into the buffer to make it null-terminated.
    address.copy(buffer, address.size());
    buffer[address.size()] = '\0';

    std::uint16_t family = (address.find(':') == std::string_view::npos ? AF_INET : AF_INET6);
    m_is_v6              = (family == AF_INET6);

    int result = inet_pton(family, buffer, &m_addr);
    if (result != 1) [[unlikely]]
        throw std::invalid_argument("Invalid IP address: " + std::string(address));
}

inet_address::inet_address(const ossia::ip_address &ip, std::uint16_t port) noexcept
    : m_family(ip.is_ipv6() ? AF_INET6 : AF_INET),
      m_port(detail::to_network_endian(port)),
      m_addr() {
    if (m_family == AF_INET) {
        std::memcpy(&m_addr.v4.address, ip.address(), sizeof(m_addr.v4.address));
    } else {
        std::memcpy(&m_addr.v6.address, ip.address(), sizeof(m_addr.v6.address));
    }
}

auto inet_address::is_ipv4() const noexcept -> bool {
    return m_family == AF_INET;
}

auto inet_address::is_ipv6() const noexcept -> bool {
    return m_family == AF_INET6;
}

auto inet_address::ip_address() const noexcept -> ossia::ip_address {
    if (is_ipv4()) {
        return ossia::ip_address(m_addr.v4.address.u8[0], m_addr.v4.address.u8[1],
                                 m_addr.v4.address.u8[2], m_addr.v4.address.u8[3]);
    } else {
        return ossia::ip_address(m_addr.v6.address[0], m_addr.v6.address[1], m_addr.v6.address[2],
                                 m_addr.v6.address[3], m_addr.v6.address[4], m_addr.v6.address[5],
                                 m_addr.v6.address[6], m_addr.v6.address[7]);
    }
}

auto inet_address::set_ip_address(const ossia::ip_address &ip) noexcept -> void {
    if (ip.is_ipv4()) {
        m_family = AF_INET;
        std::memcpy(&m_addr.v4.address, ip.address(), sizeof(m_addr.v4.address));
    } else {
        m_family = AF_INET6;
        std::memcpy(&m_addr.v6.address, ip.address(), sizeof(m_addr.v6.address));
    }
}

auto inet_address::operator==(const inet_address &other) const noexcept -> bool {
    if (m_family != other.m_family)
        return false;

    if (m_port != other.m_port)
        return false;

    if (is_ipv4())
        return std::memcmp(&m_addr.v4, &other.m_addr.v4, sizeof(m_addr.v4)) == 0;
    return std::memcmp(&m_addr.v6, &other.m_addr.v6, sizeof(m_addr.v6)) == 0;
}

auto inet_address::operator!=(const inet_address &other) const noexcept -> bool {
    return !(*this == other);
}
