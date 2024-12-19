#include "ossia/network.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#    include <WS2tcpip.h>
#    include <WinSock2.h>
#endif

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
