#pragma once

#include <helpers.hpp>
#include <coroutine>
#include <string>
#include <stdexcept>

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
            return generator(std::coroutine_handle<generator_promise_type>::from_promise(*this)); // Compiler can figure out the offset to the handle from a promise
        }

        /* Whether the coroutine suspends itself before it starts executing */
        static constexpr auto initial_suspend() noexcept { return std::suspend_always{}; }

        /* Whether the coroutine suspends itself at the end before destruction. This is done to avoid the coroutine automatic destruction */
        static constexpr auto final_suspend() noexcept { return std::suspend_always{}; }

        constexpr void unhandled_exception() noexcept {
            if constexpr(enable_exceptions_propagation) {
                value_holder_t::set_exception(std::current_exception());
            }
        }

        template<typename U = T>
        constexpr auto yield_value(U &&val) noexcept {
            value_holder_t::set_value(std::forward<U>(val));
            return std::suspend_always{};
        }

        template<typename U = T>
        constexpr auto yield_value(const U &val) noexcept {
            value_holder_t::set_value(std::forward<U>(val));
            return std::suspend_always{};
        }

        constexpr void return_void() const noexcept {}
    };

    using promise_type = generator_promise_type;

public:
    struct iterator : std::iterator<std::input_iterator_tag, T> {
        iterator(std::coroutine_handle<generator_promise_type> handle = nullptr) : it_handle_(handle) {}

        //using iterator_concept = std::input_iterator_tag;
        //using difference_type = ptrdiff_t;
        //using value_type = std::remove_cvref_t<T>;

        inline iterator &operator++() noexcept(!enable_exceptions_propagation) {
            if (it_handle_ && !it_handle_.done())it_handle_.resume();
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
                if (auto ptr = it_handle_.promise().get_exception_ptr()) {
                    it_handle_ = nullptr;
                    std::rethrow_exception(ptr);
                }
            }
        }

        inline T const &operator*() noexcept(!enable_exceptions_propagation) {
            rethrow_exceptions();
            return it_handle_.promise().get_value();
        }

        constexpr bool operator!=(const iterator &other) const noexcept {
            return it_handle_ != other.it_handle_;
        }

    private:
        std::coroutine_handle<generator_promise_type> it_handle_;

    };

    iterator begin() noexcept(!enable_exceptions_propagation) {
        if (handle_) {
            handle_.resume();
            rethrow_exceptions();
            if (done()) {
                return end();
            }
        }
        return iterator(handle_);
    }

    iterator end() const noexcept { return {}; }

    void destroy() noexcept {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    bool done() const noexcept {
        return handle_.done();
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
    std::optional<T> get() noexcept(!enable_exceptions_propagation) {
        if constexpr (enable_exceptions_propagation) {
            rethrow_exceptions();
        }
        if (handle_ && !handle_.done()) {
            return handle_.promise().get_value();
        } else {
            return std::nullopt;
        }

    }

    std::optional<T> resume() noexcept(!enable_exceptions_propagation) {
        if constexpr (enable_exceptions_propagation) {
            rethrow_exceptions();
        }
        if (handle_ && !handle_.done()) {
            handle_.resume();
            return get();
        } else {
            return std::nullopt;
        }
    }

    auto operator()() noexcept(!enable_exceptions_propagation) {
        return resume();
    }

public:
    operator std::coroutine_handle<generator_promise_type>() const { return handle_; }

    // A coroutine_handle<promise_type> converts to coroutine_handle<>
    operator std::coroutine_handle<>() const { return handle_; }

public:
    generator() = default;

    explicit generator(std::coroutine_handle<generator_promise_type> handle) : handle_(handle) {}

    generator(const generator &) = delete;

    generator(generator &&other) noexcept: handle_(std::exchange(other.handle_, nullptr)) {}

    generator &operator=(const generator &) = delete;

    generator &operator=(generator &&other) noexcept {
        if (&other != this) {
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }

    ~generator() {
        destroy();
    }


private:
    std::coroutine_handle<generator_promise_type> handle_;

};


static constexpr void static_tests_coroutine_generator() {
    static_assert(sizeof(generator<int, true>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(generator<int, false>) == sizeof(std::coroutine_handle<void>));

    static_assert(sizeof(generator<int, false>::promise_type) == sizeof(int));
    static_assert(sizeof(generator<int, true>::promise_type) == sizeof(std::variant<int, std::exception_ptr>));

}