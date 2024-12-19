#pragma once

#include <bit>
#include <cstdint>
#include <string_view>

namespace ossia {
namespace detail {

/// \brief
///   Convert a 16-bit value from host endian into network endian.
/// \param value
///   The value to be converted into network endian.
/// \return
///   The value converted into network endian.
[[nodiscard]]
constexpr auto to_network_endian(std::uint16_t value) noexcept -> std::uint16_t {
    if constexpr (std::endian::native == std::endian::little) {
        return (value >> 8) | (value << 8);
    } else {
        return value;
    }
}

/// \brief
///   Convert a 32-bit value from host endian into network endian.
/// \param value
///   The value to be converted into network endian.
/// \return
///   The value converted into network endian.
[[nodiscard]]
constexpr auto to_network_endian(std::uint32_t value) noexcept -> std::uint32_t {
    if constexpr (std::endian::native == std::endian::little) {
        return (value >> 24) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | (value << 24);
    } else {
        return value;
    }
}

/// \brief
///   Convert a 16-bit value from network endian into host endian.
/// \param value
///   The value to be converted into host endian.
/// \return
///   The value converted into host endian.
[[nodiscard]]
constexpr auto to_host_endian(std::uint16_t value) noexcept -> std::uint16_t {
    if constexpr (std::endian::native == std::endian::little) {
        return (value >> 8) | (value << 8);
    } else {
        return value;
    }
}

/// \brief
///   Convert a 32-bit value from network endian into host endian.
/// \param value
///   The value to be converted into host endian.
/// \return
///   The value converted into host endian.
[[nodiscard]]
constexpr auto to_host_endian(std::uint32_t value) noexcept -> std::uint32_t {
    if constexpr (std::endian::native == std::endian::little) {
        return (value >> 24) | ((value >> 8) & 0xFF00) | ((value << 8) & 0xFF0000) | (value << 24);
    } else {
        return value;
    }
}

} // namespace detail

/// \class ip_address
/// \brief
///   Represents an IP address. Both IPv4 and IPv6 are supported.
class ip_address {
public:
    /// \brief
    ///   Create an empty IP address. An empty IP address is a zero-initialized IPv4 address.
    constexpr ip_address() noexcept : m_is_v6(), m_addr() {}

    /// \brief
    ///   Create an IPv4 address.
    /// \param v0
    ///   The first byte of the address.
    /// \param v1
    ///   The second byte of the address.
    /// \param v2
    ///   The third byte of the address.
    /// \param v3
    ///   The fourth byte of the address.
    constexpr ip_address(std::uint8_t v0,
                         std::uint8_t v1,
                         std::uint8_t v2,
                         std::uint8_t v3) noexcept
        : m_is_v6(),
          m_addr() {
        m_addr.v4.u8[0] = v0;
        m_addr.v4.u8[1] = v1;
        m_addr.v4.u8[2] = v2;
        m_addr.v4.u8[3] = v3;
    }

    /// \brief
    ///   Create an IPv6 address.
    /// \param v0
    ///   The first 16-bit value of the address in native endian.
    /// \param v1
    ///   The second 16-bit value of the address in native endian.
    /// \param v2
    ///   The third 16-bit value of the address in native endian.
    /// \param v3
    ///   The fourth 16-bit value of the address in native endian.
    /// \param v4
    ///   The fifth 16-bit value of the address in native endian.
    /// \param v5
    ///   The sixth 16-bit value of the address in native endian.
    /// \param v6
    ///   The seventh 16-bit value of the address in native endian.
    /// \param v7
    ///   The eighth 16-bit value of the address in native endian.
    constexpr ip_address(std::uint16_t v0,
                         std::uint16_t v1,
                         std::uint16_t v2,
                         std::uint16_t v3,
                         std::uint16_t v4,
                         std::uint16_t v5,
                         std::uint16_t v6,
                         std::uint16_t v7) noexcept
        : m_is_v6(true),
          m_addr() {
        m_addr.v6.u16[0] = detail::to_network_endian(v0);
        m_addr.v6.u16[1] = detail::to_network_endian(v1);
        m_addr.v6.u16[2] = detail::to_network_endian(v2);
        m_addr.v6.u16[3] = detail::to_network_endian(v3);
        m_addr.v6.u16[4] = detail::to_network_endian(v4);
        m_addr.v6.u16[5] = detail::to_network_endian(v5);
        m_addr.v6.u16[6] = detail::to_network_endian(v6);
        m_addr.v6.u16[7] = detail::to_network_endian(v7);
    }

    /// \brief
    ///   Create an IP address from a string.
    /// \param address
    ///   The string representation of the address.
    /// \throws std::invalid_argument
    ///   Thrown if \p address is neither a valid IPv4 nor a valid IPv6 address.
    OSSIA_API ip_address(std::string_view address);

    /// \brief
    ///   \c ip_address is trivially copyable.
    constexpr ip_address(const ip_address &other) noexcept = default;

    /// \brief
    ///   \c ip_address is trivially movable.
    constexpr ip_address(ip_address &&other) noexcept = default;

    /// \brief
    ///   \c ip_address is trivially destructible.
    ~ip_address() = default;

    /// \brief
    ///   \c ip_address is trivially copyable.
    constexpr auto operator=(const ip_address &other) noexcept -> ip_address & = default;

    /// \brief
    ///   \c ip_address is trivially movable.
    constexpr auto operator=(ip_address &&other) noexcept -> ip_address & = default;

    /// \brief
    ///   Checks if this is an IPv4 address. An \c ip_address object is either an IPv4 or an IPv6
    ///   address.
    /// \retval true
    ///   This is an IPv4 address.
    /// \retval false
    ///   This is an IPv6 address.
    [[nodiscard]]
    constexpr auto is_ipv4() const noexcept -> bool {
        return !m_is_v6;
    }

    /// \brief
    ///   Checks if this is an IPv6 address. An \c ip_address object is either an IPv4 or an IPv6
    /// \retval true
    ///   This is an IPv6 address.
    /// \retval false
    ///   This is an IPv4 address.
    [[nodiscard]]
    constexpr auto is_ipv6() const noexcept -> bool {
        return m_is_v6;
    }

    /// \brief
    ///   Get pointer to the raw address data. Length of the address data is determined by address
    ///   type. For IPv4, the length is 4 bytes. IP address data is stored in network endian.
    /// \return
    ///   Pointer to the raw IP address data.
    [[nodiscard]]
    constexpr auto address() noexcept -> void * {
        return &m_addr;
    }

    /// \brief
    ///   Get pointer to the raw address data. Length of the address data is determined by address
    ///   type. For IPv4, the length is 4 bytes. IP address data is stored in network endian.
    /// \return
    ///   Pointer to the raw IP address data.
    [[nodiscard]]
    constexpr auto address() const noexcept -> const void * {
        return &m_addr;
    }

    /// \brief
    ///   Checks if this address is an IPv4 loopback address. IPv4 loopback address is \c 127.0.0.1.
    /// \retval true
    ///   This address is an IPv4 loopback address.
    /// \retval false
    ///   This address is not an IPv4 loopback address.
    [[nodiscard]]
    constexpr auto is_ipv4_loopback() const noexcept -> bool {
        if (!is_ipv4())
            return false;

        return m_addr.v4.u8[0] == 127 && m_addr.v4.u8[1] == 0 && m_addr.v4.u8[2] == 0 &&
               m_addr.v4.u8[3] == 1;
    }

    /// \brief
    ///   Checks if this address is an IPv4 any address. IPv4 any address is \c 0.0.0.0.
    /// \retval true
    ///   This address is an IPv4 any address.
    /// \retval false
    ///   This address is not an IPv4 any address.
    [[nodiscard]]
    constexpr auto is_ipv4_any() const noexcept -> bool {
        return is_ipv4() && m_addr.v4.u32[0] == 0;
    }

    /// \brief
    ///   Checks if this address is an IPv4 broadcast address. IPv4 broadcast address is
    ///   \c 255.255.255.255.
    /// \retval true
    ///   This address is an IPv4 broadcast address.
    /// \retval false
    ///   This address is not an IPv4 broadcast address.
    [[nodiscard]]
    constexpr auto is_ipv4_broadcast() const noexcept -> bool {
        return is_ipv4() && m_addr.v4.u32[0] == 0xFFFFFFFFU;
    }

    /// \brief
    ///   Checks if this address is an IPv4 private address. An IPv4 private network is a network
    ///   that used for local area networks. Private address ranges are defined in RFC 1918 as
    ///   follows:
    ///   - \c 10.0.0.0/8
    ///   - \c 172.16.0.0/12
    ///   - \c 192.168.0.0/16
    /// \retval true
    ///   This address is an IPv4 private address.
    /// \retval false
    ///   This address is not an IPv4 private address.
    [[nodiscard]]
    constexpr auto is_ipv4_private() const noexcept -> bool {
        if (!is_ipv4())
            return false;

        // 10.0.0.0/8
        if (m_addr.v4.u8[0] == 10)
            return true;

        // 172.16.0.0/12
        if (m_addr.v4.u8[0] == 172 && (m_addr.v4.u8[1] & 0xF0) == 16)
            return true;

        // 192.168.0.0/16
        if (m_addr.v4.u8[0] == 192 && m_addr.v4.u8[1] == 168)
            return true;

        return false;
    }

    /// \brief
    ///   Checks if this address is an IPv4 link local address. IPv4 link local address is
    ///   \c 169.254.0.0/16 as defined in RFC 3927.
    /// \retval true
    ///   This address is an IPv4 link local address.
    /// \retval false
    ///   This address is not an IPv4 link local address.
    [[nodiscard]]
    constexpr auto is_ipv4_link_local() const noexcept -> bool {
        if (!is_ipv4())
            return false;

        return m_addr.v4.u8[0] == 169 && m_addr.v4.u8[1] == 254;
    }

    /// \brief
    ///   Checks if this address is an IPv4 multicast address. IPv4 multicast address is \c
    ///   224.0.0.0/4 as defined in RFC 5771.
    /// \retval true
    ///   This address is an IPv4 multicast address.
    /// \retval false
    ///   This address is not an IPv4 multicast address.
    [[nodiscard]]
    constexpr auto is_ipv4_multicast() const noexcept -> bool {
        if (!is_ipv4())
            return false;

        return (m_addr.v4.u8[0] & 0xF0) == 224;
    }

    /// \brief
    ///   Checks if this address is an IPv6 loopback address. IPv6 loopback address is \c ::1.
    /// \retval true
    ///   This address is an IPv6 loopback address.
    /// \retval false
    ///   This address is not an IPv6 loopback address.
    [[nodiscard]]
    constexpr auto is_ipv6_loopback() const noexcept -> bool {
        if (!is_ipv6())
            return false;

        return m_addr.v6.u16[0] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[1] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[2] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[3] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[4] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[5] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[6] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[7] == detail::to_network_endian(std::uint16_t(1));
    }

    /// \brief
    ///   Checks if this address is an IPv6 any address. IPv6 any address is \c ::.
    /// \retval true
    ///   This address is an IPv6 any address.
    /// \retval false
    ///   This address is not an IPv6 any address.
    [[nodiscard]]
    constexpr auto is_ipv6_any() const noexcept -> bool {
        if (!is_ipv6())
            return false;

        return m_addr.v6.u32[0] == 0 && m_addr.v6.u32[1] == 0 && m_addr.v6.u32[2] == 0 &&
               m_addr.v6.u32[3] == 0;
    }

    /// \brief
    ///   Checks if this address is an IPv6 multicast address. IPv6 multicast address is \c FF00::/8
    ///   as defined in RFC 4291.
    /// \retval true
    ///   This address is an IPv6 multicast address.
    /// \retval false
    ///   This address is not an IPv6 multicast address.
    [[nodiscard]]
    constexpr auto is_ipv6_multicast() const noexcept -> bool {
        if (!is_ipv6())
            return false;

        return (m_addr.v6.u8[0] & 0xFF) == 0xFF;
    }

    /// \brief
    ///   Checks if this address is an IPv4 mapped IPv6 address. IPv4 mapped IPv6 address is
    ///   \c ::FFFF:0:0/96.
    /// \retval true
    ///   This is an IPv4 mapped IPv6 address.
    /// \retval false
    ///   This is not an IPv4 mapped IPv6 address.
    [[nodiscard]]
    constexpr auto is_ipv4_mapped_ipv6() const noexcept -> bool {
        if (!is_ipv6())
            return false;

        return m_addr.v6.u16[0] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[1] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[2] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[3] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[4] == detail::to_network_endian(std::uint16_t(0)) &&
               m_addr.v6.u16[5] == detail::to_network_endian(std::uint16_t(0xFFFF));
    }

    /// \brief
    ///   Converts this IP address to IPv4 address. It is undefined behavior if this is neither an
    ///   IPv4 address nor an IPv4-mapped IPv6 address.
    /// \return
    ///   Return this address if this is an IPv4 or IPv4-mapped IPv6 address.
    [[nodiscard]]
    constexpr auto to_ipv4() const noexcept -> ip_address {
        if (is_ipv4())
            return *this;

        return ip_address(m_addr.v6.u8[12], m_addr.v6.u8[13], m_addr.v6.u8[14], m_addr.v6.u8[15]);
    }

    /// \brief
    ///   Converts this IP address to IPv6 address.
    /// \return
    ///   Return an IPv4-mapped IPv6 address if this is an IPv4 address. Otherwise, return this IPv6
    ///   address itself.
    [[nodiscard]]
    constexpr auto to_ipv6() const noexcept -> ip_address {
        if (is_ipv6())
            return *this;

        return ip_address(0, 0, 0, 0, 0, 0xFFFF, m_addr.v4.u16[0], m_addr.v4.u16[1]);
    }

    /// \brief
    ///   Checks if this \c ip_address is the same as another one.
    /// \param other
    ///   The \c ip_address to be compared with.
    /// \retval true
    ///   This \c ip_address is the same as \p other.
    /// \retval false
    ///   This \c ip_address is different from \p other.
    [[nodiscard]]
    constexpr auto operator==(const ip_address &other) const noexcept -> bool {
        if (m_is_v6 != other.m_is_v6)
            return false;

        if (is_ipv4())
            return m_addr.v4.u32[0] == other.m_addr.v4.u32[0];

        return m_addr.v6.u32[0] == other.m_addr.v6.u32[0] &&
               m_addr.v6.u32[1] == other.m_addr.v6.u32[1] &&
               m_addr.v6.u32[2] == other.m_addr.v6.u32[2] &&
               m_addr.v6.u32[3] == other.m_addr.v6.u32[3];
    }

    /// \brief
    ///   Checks if this \c ip_address is different from another one.
    /// \param other
    ///   The \c ip_address to be compared with.
    /// \retval true
    ///   This \c ip_address is different from \p other.
    /// \retval false
    ///   This \c ip_address is the same as \p other.
    [[nodiscard]]
    constexpr auto operator!=(const ip_address &other) const noexcept -> bool {
        return !(*this == other);
    }

private:
    bool m_is_v6;
    union {
        union {
            std::uint8_t  u8[4];
            std::uint16_t u16[2];
            std::uint32_t u32[1];
        } v4;
        union {
            std::uint8_t  u8[16];
            std::uint16_t u16[8];
            std::uint32_t u32[4];
        } v6;
    } m_addr;
};

/// \brief
///   IPv4 loopback address.
inline constexpr ip_address ipv4_loopback{127, 0, 0, 1};

/// \brief
///   IPv4 any address.
inline constexpr ip_address ipv4_any{0, 0, 0, 0};

/// \brief
///   IPv4 broadcast address.
inline constexpr ip_address ipv4_broadcast{255, 255, 255, 255};

/// \brief
///   IPv6 loopback address.
inline constexpr ip_address ipv6_loopback{0, 0, 0, 0, 0, 0, 0, 1};

/// \brief
///   IPv6 any address.
inline constexpr ip_address ipv6_any{0, 0, 0, 0, 0, 0, 0, 0};

/// \class inet_address
/// \brief
///   Wrapper class for Internet socket address. \c inet_address is a trivial class. This class
///   could be directly passed as \c sockaddr to system socket API.
class inet_address {
public:
    /// \brief
    ///   Create an empty Internet socket address. An empty \c inet_address object is trivially
    ///   initialized with random values and should not be used for network operations.
    inet_address() noexcept = default;

    /// \brief
    ///   Create an Internet socket address with IP address and port number.
    /// \param ip
    ///   The IP address of the Internet socket address.
    /// \param port
    ///   The port number of the Internet socket address in host endian.
    OSSIA_API inet_address(const ossia::ip_address &ip, std::uint16_t port) noexcept;

    /// \brief
    ///   \c inet_address is trivially copyable.
    inet_address(const inet_address &other) noexcept = default;

    /// \brief
    ///   \c inet_address is trivially movable.
    inet_address(inet_address &&other) noexcept = default;

    /// \brief
    ///   \c inet_address is trivially destructible.
    ~inet_address() = default;

    /// \brief
    ///   \c inet_address is trivially copyable.
    auto operator=(const inet_address &other) noexcept -> inet_address & = default;

    /// \brief
    ///   \c inet_address is trivially movable.
    auto operator=(inet_address &&other) noexcept -> inet_address & = default;

    /// \brief
    ///   Checks if this is an IPv4 Internet socket address.
    /// \note
    ///   Empty \c inet_address object may be neither IPv4 nor IPv6.
    /// \retval true
    ///   This is an IPv4 Internet socket address.
    /// \retval false
    ///   This is not an IPv4 Internet socket address.
    [[nodiscard]]
    OSSIA_API auto is_ipv4() const noexcept -> bool;

    /// \brief
    ///   Checks if this is an IPv6 Internet socket address.
    /// \note
    ///   Empty \c inet_address object may be neither IPv4 nor IPv6.
    /// \retval true
    ///   This is an IPv6 Internet socket address.
    /// \retval false
    ///   This is not an IPv6 Internet socket address.
    [[nodiscard]]
    OSSIA_API auto is_ipv6() const noexcept -> bool;

    /// \brief
    ///   Get IP address of this Internet socket address. It is undefined behavior if this is an
    ///   empty \c inet_address object.
    /// \return
    ///   The IP address of this Internet socket address.
    [[nodiscard]]
    OSSIA_API auto ip_address() const noexcept -> ossia::ip_address;

    /// \brief
    ///   Set IP address of this Internet socket address.
    /// \param ip
    ///   The IP address to be set.
    OSSIA_API auto set_ip_address(const ossia::ip_address &ip) noexcept -> void;

    /// \brief
    ///   Get port number of this Internet socket address.
    /// \return
    ///   The port number of this Internet socket address in host endian.
    [[nodiscard]]
    auto port() const noexcept -> std::uint16_t {
        return detail::to_host_endian(m_port);
    }

    /// \brief
    ///   Set port number of this Internet socket address.
    /// \param port
    ///   The port number to be set in host endian.
    auto set_port(std::uint16_t port) noexcept -> void {
        m_port = detail::to_network_endian(port);
    }

    /// \brief
    ///   Get IPv6 flow information of this Internet socket address. It is undefined behavior if
    ///   this is not an IPv6 Internet socket address.
    /// \return
    ///   The flow information of this Internet socket address in host endian.
    [[nodiscard]]
    auto flowinfo() const noexcept -> std::uint32_t {
        return detail::to_host_endian(m_addr.v6.flowinfo);
    }

    /// \brief
    ///   Set IPv6 flow information of this Internet socket address. It is undefined behavior if
    ///   this is not an IPv6 Internet socket address.
    /// \param flowinfo
    ///   The flow information to be set in host endian.
    auto set_flowinfo(std::uint32_t flowinfo) noexcept -> void {
        m_addr.v6.flowinfo = detail::to_network_endian(flowinfo);
    }

    /// \brief
    ///   Get IPv6 scope ID of this Internet socket address. It is undefined behavior if this is
    ///   not an IPv6 Internet socket address.
    /// \return
    ///   The scope ID of this Internet socket address in host endian.
    [[nodiscard]]
    auto scope_id() const noexcept -> std::uint32_t {
        return detail::to_host_endian(m_addr.v6.scope_id);
    }

    /// \brief
    ///   Set IPv6 scope ID of this Internet socket address. It is undefined behavior if this is
    ///   not an IPv6 Internet socket address.
    /// \param scope_id
    ///   The scope ID to be set in host endian.
    auto set_scope_id(std::uint32_t scope_id) noexcept -> void {
        m_addr.v6.scope_id = detail::to_network_endian(scope_id);
    }

    /// \brief
    ///   Checks if this Internet socket address is the same as another one.
    /// \param other
    ///   The Internet socket address to be compared with.
    /// \retval true
    ///   This Internet socket address is the same as \p other.
    /// \retval false
    ///   This Internet socket address is different from \p other.
    [[nodiscard]]
    OSSIA_API auto operator==(const inet_address &other) const noexcept -> bool;

    /// \brief
    ///   Checks if this Internet socket address is different from another one.
    /// \param other
    ///   The Internet socket address to be compared with.
    /// \retval true
    ///   This Internet socket address is different from \p other.
    /// \retval false
    ///   This Internet socket address is the same as \p other.
    [[nodiscard]]
    OSSIA_API auto operator!=(const inet_address &other) const noexcept -> bool;

private:
    std::uint16_t m_family;
    std::uint16_t m_port;
    union {
        struct {
            union {
                std::uint8_t  u8[4];
                std::uint16_t u16[2];
                std::uint32_t u32[1];
            } address;
            std::uint8_t zero[8];
        } v4;
        struct {
            std::uint32_t flowinfo;
            std::uint16_t address[8];
            std::uint32_t scope_id;
        } v6;
    } m_addr;
};

} // namespace ossia
