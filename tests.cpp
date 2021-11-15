#include <coro>
#include <iostream>

template<typename T>
generator<T, true> range(T begin, T end, T step = 1) {
    for (T i = begin; i < end; i += step) {
        co_yield i; // == co_await p.yield_value(e);
        if (step == 0) throw std::runtime_error("Step set to 0 in range."s);
    }
}

void check_range_exceptions() {

    auto gen = range<float>(1, 10, 0);
    assert(gen() == 1);
    try {
        gen();
        assert(false);
    } catch (std::exception &e) {
        assert(std::string(e.what()) == "Step set to 0 in range."s);
        std::cout << e.what() << std::endl;
    }

    try {
        gen();
        assert(false);
    } catch (std::exception &e) {
        assert(std::string(e.what()) == "Called coroutine on empty/destroyed handle"s);
        std::cout << e.what() << std::endl;
    }
}


int main() {
    check_range_exceptions();
}