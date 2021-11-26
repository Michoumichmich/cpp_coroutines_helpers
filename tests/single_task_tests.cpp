#include <coro>
#include "helpers.hpp"


template<bool start_immediately, bool propagate_exceptions>
single_task<int, start_immediately, propagate_exceptions> state_machine(int steps) {
    if (steps < 0) {
        throw std::runtime_error("Negative number of steps");
    }
    for (int i = 0; i < steps; ++i) {
        std::cout << i << std::endl;
        co_await std::suspend_always{};
        if (i > 4) throw std::runtime_error("Too many steps");
    }
    co_return steps;
}


TEST(state_machine, immediate_start_step_count) {
    int counter = 0;
    auto machine = state_machine<true, false>(3);
    for (auto res = machine(); !res.has_value(); counter++, res = machine());
    ASSERT_EQ(counter, 2);
    ASSERT_TRUE(machine.get());
    ASSERT_EQ(*machine.get(), 3);
}


TEST(state_machine, not_immediate_start_step_count) {
    int counter = 0;
    auto machine = state_machine<false, true>(3);
    for (auto res = machine(); !res.has_value(); counter++, res = machine());
    ASSERT_EQ(counter, 3);
    ASSERT_TRUE(machine.get());
    ASSERT_EQ(*machine.get(), 3);
}


TEST(state_machine, not_immediate_start_throw_begining) {
    auto machine = state_machine<false, true>(-1);
    EXPECT_THROW_RUNTIME_ERROR_STREQ(machine();, "Negative number of steps");
    EXPECT_THROW_RUNTIME_ERROR_STREQ(machine();, "Called coroutine on empty/destroyed handle");
}

TEST(state_machine, immediate_start_throw_begining) {
    auto machine = state_machine<true, true>(-1); // Exception thrown here
    EXPECT_THROW_RUNTIME_ERROR_STREQ(machine.get();, "Negative number of steps"); // on each call, we try to throw the pending exception, then run the coroutine and throw a new exception if there's one.
    EXPECT_THROW_RUNTIME_ERROR_STREQ(machine();, "Called coroutine on empty/destroyed handle");
}

TEST(state_machine, immediate_start_throw_begining_no_propagate) {
    auto machine = state_machine<true, false>(-1); // Exception thrown here
    ASSERT_TRUE(machine()); // Well, there was an exception thrown, but the coroutine appears as "done" as we don't handle the exceptions inside. The return value is garbage (undefined).

    std::cout << *machine.get() << std::endl;
}


TEST(single_task, static_test) {
    static_tests_single_task();
}