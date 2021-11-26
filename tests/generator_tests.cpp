#include "helpers.hpp"
#include <coro>


#include <numeric>

template<typename T, bool propagate_exceptions>
generator<T, propagate_exceptions> range(T begin, T end, T step = 1) {
    if (step == 0) throw std::runtime_error("Step set to 0 in range."s);
    for (T i = begin; i < end; i += step) {
        co_yield i; // == co_await p.yield_value(e);
    }
}

template<typename T, bool propagate_exceptions>
generator<T, propagate_exceptions> range_throw_on_success(T begin, T end, T step = 1) {
    if (step == 0) throw std::runtime_error("Step set to 0 in range."s);
    for (T i = begin; i < end; i += step) {
        co_yield i; // == co_await p.yield_value(e);
    }
    throw std::runtime_error("Done!"s);

}


TEST(range, exceptions_propagation_enabled) {
    auto gen = range<float, true>(1, 10, 0);
    EXPECT_THROW_RUNTIME_ERROR_STREQ(gen();, "Step set to 0 in range.");
    EXPECT_THROW_RUNTIME_ERROR_STREQ(gen();, "Called coroutine on empty/destroyed handle");  // After the exception, the coroutine handle is destroyed, it should be called again.
}

TEST(range, exceptions_propagation_enabled_iterator) {
    auto gen = range<float, true>(1, 10, 0);
    generator<float, true>::iterator it;

    EXPECT_THROW_RUNTIME_ERROR_STREQ(it = gen.begin();, "Step set to 0 in range.");
    EXPECT_THROW_RUNTIME_ERROR_STREQ(*it;, "Called coroutine on empty/destroyed handle");  // After the exception, the coroutine handle is destroyed, it should be called again.
    EXPECT_THROW_RUNTIME_ERROR_STREQ(it = ++it;, "Called coroutine on empty/destroyed handle");  // After the exception, the coroutine handle is destroyed, it should be called again.

    ASSERT_FALSE(it != gen.end());

    auto empty = range_throw_on_success<int, true>(0, 100, 1);
    EXPECT_THROW_RUNTIME_ERROR_STREQ({ ASSERT_EQ(std::accumulate(empty.begin(), empty.end(), 0), 99 * 100 / 2); }, "Done!");
}

TEST(range, exceptions_propagation_disabled) {
    auto gen = range<int, false>(1, 10, 0);
    EXPECT_NO_THROW(gen(););
    ASSERT_TRUE(gen.done());
    ASSERT_FALSE(gen());
    ASSERT_FALSE(gen.get());
    ASSERT_FALSE(gen.resume());
}

TEST(range, integer_sum_range_for_loop) {
    int acc = 0;
    for (auto &i: range<int, true>(0, 100, 1)) {
        acc += i;
    }
    ASSERT_EQ(acc, (99 * 100) / 2);
}

TEST(range, integer_sum_std_accumulate) {
    auto generator = range<int, true>(0, 100, 1);
    ASSERT_EQ(std::accumulate(generator.begin(), generator.end(), 0), (99 * 100) / 2);
}

TEST(range, integer_sum_while_loop) {
    int acc = 0;
    auto generator = range<int, true>(0, 100, 1);
    while (auto v = generator()) {
        acc += *v;
    }
    ASSERT_EQ(acc, (99 * 100) / 2);
}

TEST(range, empty_range) {
    auto empty = range<int, true>(0, 0, 1);
    ASSERT_EQ(std::accumulate(empty.begin(), empty.end(), 0), 0);
}


TEST(generator, static_tests) {
    static_tests_coroutine_generator();
}