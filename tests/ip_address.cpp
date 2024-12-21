#include "ossia/inet_address.hpp"

#include <doctest/doctest.h>

#include <stdexcept>
#include <tuple>

using namespace ossia;

TEST_CASE("IPv4 loopback ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("127.0.0.1"));
    CHECK_THROWS_AS(std::ignore = ip_address(""), std::invalid_argument &);
    CHECK_THROWS_AS(std::ignore = ip_address("255.123.255.345"), std::invalid_argument &);

    ip_address addr(127, 0, 0, 1);
    CHECK(addr == ipv4_loopback);
    CHECK(addr == ip_address("127.0.0.1"));

    CHECK(addr.is_ipv4());
    CHECK(addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv4 any ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("0.0.0.0"));

    ip_address addr(0, 0, 0, 0);
    CHECK(addr == ipv4_any);
    CHECK(addr == ip_address("0.0.0.0"));

    CHECK(addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv4 broadcast ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("255.255.255.255"));

    ip_address addr(255, 255, 255, 255);
    CHECK(addr == ipv4_broadcast);
    CHECK(addr == ip_address("255.255.255.255"));

    CHECK(addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv4 private ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("192.168.0.1"));

    ip_address addr(192, 168, 0, 1);
    CHECK(addr == ip_address("192.168.0.1"));

    CHECK(addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv4 link-local ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("169.254.0.1"));

    ip_address addr(169, 254, 0, 1);
    CHECK(addr == ip_address("169.254.0.1"));

    CHECK(addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv4 multicast ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("224.0.0.255"));

    ip_address addr(224, 0, 0, 255);
    CHECK(addr == ip_address("224.0.0.255"));

    CHECK(addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(addr.is_ipv4_multicast());

    CHECK(!addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == addr);
}

TEST_CASE("IPv6 loopback ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("::1"));

    ip_address addr(0, 0, 0, 0, 0, 0, 0, 1);
    CHECK(addr == ipv6_loopback);
    CHECK(addr == ip_address("::1"));

    CHECK(!addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(addr.is_ipv6());
    CHECK(addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());
    CHECK(addr.to_ipv6() == addr);
}

TEST_CASE("IPv6 any ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("::"));

    ip_address addr(0, 0, 0, 0, 0, 0, 0, 0);
    CHECK(addr == ip_address("::"));

    CHECK(!addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());
    CHECK(addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());
    CHECK(addr.to_ipv6() == addr);
}

TEST_CASE("IPv6 multicast ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("FF00::1"));

    ip_address addr(0xFF00, 0, 0, 0, 0, 0, 0, 1);
    CHECK(addr == ip_address("FF00::1"));

    CHECK(!addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(addr.is_ipv6_multicast());
    CHECK(!addr.is_ipv4_mapped_ipv6());
    CHECK(addr.to_ipv6() == addr);
}

TEST_CASE("IPv4 mapped IPv6 ip_address") {
    CHECK_NOTHROW(std::ignore = ip_address("::FFFF:FFFF:FFFF"));

    ip_address addr(0, 0, 0, 0, 0, 0xFFFF, 0xFFFF, 0xFFFF);
    CHECK(addr == ip_address("::FFFF:FFFF:FFFF"));

    CHECK(!addr.is_ipv4());
    CHECK(!addr.is_ipv4_loopback());
    CHECK(!addr.is_ipv4_any());
    CHECK(!addr.is_ipv4_broadcast());
    CHECK(!addr.is_ipv4_private());
    CHECK(!addr.is_ipv4_link_local());
    CHECK(!addr.is_ipv4_multicast());

    CHECK(addr.is_ipv6());
    CHECK(!addr.is_ipv6_loopback());
    CHECK(!addr.is_ipv6_any());
    CHECK(!addr.is_ipv6_multicast());
    CHECK(addr.is_ipv4_mapped_ipv6());

    CHECK(addr.to_ipv4() == ipv4_broadcast);
    CHECK(addr.to_ipv6() == addr);
}
