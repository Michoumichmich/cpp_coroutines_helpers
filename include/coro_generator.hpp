#pragma once

#include <helpers.hpp>
#include <experimental/coroutine>
#include <string>

using namespace std::string_literals;

template<typename T, bool enable_exceptions_propagation = true>
struct generator {

    static_assert(!std::is_void_v<T>);
public:
    /**
     * The promise will be stored in the coroutine execution context along the variables,
     * registers, instruction pointer, parameters, all of the function state (lambdas too).
     */
    struct generator_promise_type : value_holder<T, enable_exceptions_propagation> {
        using value_holder_t = value_holder<T, enable_exceptions_propagation>;
    public:

        /* Called by the compiler to convert the promise type into the return object, for the caller. */
        generator get_return_object() noexcept {
            return generator(std::experimental::coroutine_handle<generator_promise_type>::from_promise(*this)); // Compiler can figure out the offset to the handle from a promise
        }

        /* Whether the coroutine suspends itself before it starts executing */
        static constexpr auto initial_suspend() noexcept { return std::experimental::suspend_always{}; }

        /* Whether the coroutine suspends itself at the end before destruction. This is done to avoid the coroutine automatic destruction */
        static constexpr auto final_suspend() noexcept { return std::experimental::suspend_always{}; }

        constexpr void unhandled_exception() noexcept {
            if constexpr(enable_exceptions_propagation) {
                value_holder_t::set_exception(std::current_exception());
            }
        }

        template<typename U = T>
        constexpr auto yield_value(U &&val) noexcept {
            value_holder_t::set_value(std::forward<U>(val));
            return std::experimental::suspend_always{};
        }

        template<typename U = T>
        constexpr auto yield_value(const U &val) noexcept {
            value_holder_t::set_value(std::forward<U>(val));
            return std::experimental::suspend_always{};
        }

        constexpr void return_void() const noexcept {}
    };

    using promise_type = generator_promise_type;

public:
    struct iterator : std::iterator<std::input_iterator_tag, T> {
        iterator(std::experimental::coroutine_handle<generator_promise_type> handle) : it_handle_(handle) {}

        inline iterator &operator++() noexcept(!enable_exceptions_propagation) {
            if (it_handle_)it_handle_.resume();
            rethrow_exceptions();
            if (it_handle_.done()) { it_handle_ = nullptr; }
            return *this;
        }

        inline void rethrow_exceptions() noexcept(!enable_exceptions_propagation) {
            if constexpr(!enable_exceptions_propagation) {
                return;
            } else {
                if (!it_handle_) {
                    throw std::runtime_error("Called coroutine on empty/destroyed handle"s);
                }
                auto ptr = it_handle_.promise().get_exception_ptr();
                if (ptr) {
                    it_handle_ = nullptr;
                    std::rethrow_exception(ptr);
                }
            }
        }

        inline T const &operator*() noexcept(!enable_exceptions_propagation) {
            rethrow_exceptions();
            return it_handle_.promise().get_value();
        }

        constexpr bool operator!=(const iterator &other) noexcept {
            return it_handle_ != other.it_handle_;
        }

    private:
        std::experimental::coroutine_handle<generator_promise_type> it_handle_;

    };

    iterator begin() noexcept(!enable_exceptions_propagation) {
        if (handle_) {
            handle_.resume();
            rethrow_exceptions();
            if (handle_.done()) {
                return end();
            }
        }
        return iterator(handle_);
    }

    iterator end() noexcept { return iterator(nullptr); }

    void destroy() noexcept {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

private:
    void rethrow_exceptions() noexcept(!enable_exceptions_propagation) {
        if constexpr(!enable_exceptions_propagation) {
            return;
        } else {
            if (!handle_) {
                throw std::runtime_error("Called coroutine on empty/destroyed handle"s);
            }
            auto ptr = handle_.promise().get_exception_ptr();
            if (ptr) {
                destroy();
                std::rethrow_exception(ptr);
            }
        }
    }

public:
    T const &get() noexcept(!enable_exceptions_propagation) {
        rethrow_exceptions();
        return handle_.promise().get_value();
    }

    T const &operator()() noexcept(!enable_exceptions_propagation) {
        rethrow_exceptions();
        handle_.resume();
        return get();
    }

public:
    operator std::experimental::coroutine_handle<generator_promise_type>() const { return handle_; }

    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    operator std::experimental::coroutine_handle<>() const { return handle_; }

public:
    generator() = default;

    explicit generator(std::experimental::coroutine_handle<generator_promise_type> handle) : handle_(handle) {}

    generator(const generator &) = delete;

    generator(generator &&other) noexcept: handle_(other.handle_) { other.handle_ = nullptr; }

    generator &operator=(const generator &) = delete;

    generator &operator=(generator &&other) noexcept {
        if (&other != this) {
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~generator() {
        destroy();
    }


private:
    std::experimental::coroutine_handle<generator_promise_type> handle_;

};


static constexpr void static_tests_coroutine_genrator() {
    static_assert(sizeof(generator<int, true>) == sizeof(std::experimental::coroutine_handle<void>));
    static_assert(sizeof(generator<int, false>) == sizeof(std::experimental::coroutine_handle<void>));

    static_assert(sizeof(generator<int, false>::promise_type) == sizeof(int));
    static_assert(sizeof(generator<int, true>::promise_type) == sizeof(std::variant<int, std::exception_ptr>));

}