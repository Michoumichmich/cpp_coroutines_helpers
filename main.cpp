#include <iostream>
#include <numeric>
#include <cassert>
#include <coro>

template<typename T>
generator<T, true> range(T begin, T end, T step = 1) {
    if (step == 0) throw std::runtime_error("Step set to 0 in range."s);
    for (T i = begin; i < end; i += step) {
        co_yield i; // == co_await p.yield_value(e);
    }
}

template<typename T>
generator<T, true> linspace(T begin, T end, T num = 1) noexcept {
    if (num == 0) {
        co_yield begin;
        co_return;
    }
    auto step = (double) (end - begin) / num;
    for (T i = begin; i < end; i += step) {
        co_yield i; // == co_await p.yield_value(e);
    }
}


void check_range_throw() {
    try {
        for (auto i: range<int>(0, 10, 0)) {
            std::cout << i << std::endl;
        }
        assert(false);
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}


single_task<int, false> slow_function() {
    for (int i = 0; i < 3; ++i) {
        co_await std::suspend_always{};
    }
    co_return 42;
}


single_task<> counter2(std::string s) {
    // struct coro_context* ctxt  = new ...
    // return = ctxt->promise.get_return_object();
    // co_await ctxt->promise.initial_suspend();
    for (unsigned i = 0;; ++i) {
        std::cout << s << i << std::endl;
        co_await std::suspend_always{};
    }

    // co_await ctxt-> promise.final_suspend();
    // delete ctxt
    //co_return;
}

int main() {
    check_range_throw();

    for (auto i: linspace<float>(0, 10, 0)) {
        std::cout << i << std::endl;
    }

    for (auto i: range<float>(0, 10, 0.99)) {
        std::cout << i << std::endl;
    }

    auto float_range = range<float>(0, 100, 0.5);
    std::cout << std::accumulate(float_range.begin(), float_range.end(), 0) << std::endl;

    //    std::ranges::sort(std::vector(10, 1));
    auto gen = slow_function();
    for (auto i = gen(); !i; i = gen())
        std::cout << "Result not ready" << std::endl;
    std::cout << "Got value: " << *gen.get() << std::endl;


    auto h = counter2("Counter2: ");
    for (int i = 0; i < 3; ++i) {
        std::cout << "In main2 function\n";
        h(); // equivalent to h.resume(), work from other threads
    }

}