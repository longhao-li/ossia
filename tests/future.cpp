#include "ossia/io_context.hpp"

#include <doctest/doctest.h>

#include <string>

using namespace ossia;

static auto task2() noexcept -> future<int &> {
    static int dummy_global_value = 42;
    co_return dummy_global_value;
}

static auto task1() noexcept -> future<std::string> {
    int &value = co_await task2();
    CHECK(value == 42);
    co_return "Hello, world!";
}

static auto task0(io_context &ctx) noexcept -> future<> {
    std::string str = co_await task1();
    CHECK(str == "Hello, world!");

    int value = co_await task2();
    CHECK(value == 42);

    str = co_await task1();
    CHECK(str == "Hello, world!");

    ctx.stop();
}

TEST_CASE("future") {
    io_context ctx(1);
    ctx.dispatch(task0, ctx);
    ctx.run();
}
