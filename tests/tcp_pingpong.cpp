#include "ossia/tcp_server.hpp"

#include <doctest/doctest.h>

using namespace ossia;
using namespace std::chrono_literals;

inline constexpr std::size_t packet_count = 1000;
inline constexpr std::size_t packet_size  = 1024;
inline constexpr std::size_t buffer_size  = 1024;

static auto server(tcp_stream stream) noexcept -> future<> {
    char        buffer[buffer_size];
    std::size_t total_size = 0;

    while (total_size < packet_size * packet_count) {
        std::uint32_t recv_size = std::min(packet_size, packet_size * packet_count - total_size);

        auto result = co_await stream.receive_async(buffer, recv_size);
        CHECK(result.has_value());
        total_size += *result;

        std::uint32_t sent_size = 0;
        while (sent_size < *result) {
            result = co_await stream.send_async(buffer + sent_size, recv_size - sent_size);
            CHECK(result.has_value());
            sent_size += *result;
        }
    }
}

static auto listener(const inet_address &address) noexcept -> future<> {
    tcp_server srv;

    auto error = srv.bind(address);
    CHECK(error.value() == 0);
    CHECK(srv.local_address() == address);

    auto connection = co_await srv.accept_async();
    CHECK(connection.has_value());

    schedule(server(std::move(*connection)));
}

static auto client(io_context &ctx, const inet_address &address) noexcept -> future<> {
    tcp_stream connection;

    auto error = co_await connection.connect_async(address);
    CHECK(error.value() == 0);
    CHECK(connection.peer_address() == address);

    CHECK(connection.set_keep_alive(true).value() == 0);
    CHECK(connection.set_no_delay(true).value() == 0);
    CHECK(connection.set_send_timeout(30s).value() == 0);
    CHECK(connection.set_receive_timeout(65s).value() == 0);

    char        buffer[buffer_size]{};
    std::size_t total_size = 0;

    while (total_size < packet_size * packet_count) {
        std::uint32_t send_size = std::min(buffer_size, packet_size * packet_count - total_size);

        auto result = co_await connection.send_async(buffer, send_size);
        CHECK(result.has_value());
        total_size += *result;

        std::uint32_t recv_size = 0;
        while (recv_size < *result) {
            result = co_await connection.receive_async(buffer + recv_size, send_size - recv_size);
            CHECK(result.has_value());
            recv_size += *result;
        }
    }

    ctx.stop();
}

TEST_CASE("TCP async ping-pong") {
    io_context ctx(1);

    inet_address address(ipv6_loopback, 23333);
    ctx.dispatch(listener, address);
    ctx.dispatch(client, ctx, address);

    ctx.run();
}
