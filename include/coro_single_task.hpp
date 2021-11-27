#pragma once

#include <coroutine>
#include <optional>
#include <string>
#include <stdexcept>

using namespace std::string_literals;

#include <helpers.hpp>

template<typename T, bool start_immediately, bool enable_exceptions_propagation>
struct single_task_promise_type;

template<typename T = void, bool start_immediately = true, bool enable_exceptions_propagation = false>
struct single_task {

    /**
     * The promise will be stored in the coroutine execution context along the variables,
     * registers, instruction pointer, parameters, all of the function state (lambdas too).
     */
    using promise_type = single_task_promise_type<T, start_immediately, enable_exceptions_propagation>;

public:
    operator std::coroutine_handle<promise_type>() const noexcept { return handle_; }

    operator std::coroutine_handle<>() const noexcept { return handle_; }

private:
    void rethrow_exceptions() requires(enable_exceptions_propagation) {
        if (!handle_) {
            throw std::runtime_error("Called coroutine on empty/destroyed handle"s);
        }
        if (auto ptr = handle_.promise().get_exception_ptr()) {
            destroy();
            std::rethrow_exception(ptr);
        }
    }

public:
    void destroy() noexcept {
        if (handle_) {
            handle_.destroy();
            handle_ = nullptr;
        }
    }

    inline auto get() noexcept(!enable_exceptions_propagation) {
        if constexpr(enable_exceptions_propagation) {
            rethrow_exceptions();
        }
        if constexpr (std::is_void_v<T>) {
            return handle_.done();
        } else {
            if (handle_ && handle_.done()) {
                return std::optional<T>(handle_.promise().get_value());
            } else {
                return std::optional<T>(std::nullopt);
            }
        }
    }

    inline auto resume() noexcept(!enable_exceptions_propagation) {
        if constexpr(enable_exceptions_propagation) {
            rethrow_exceptions();
        }
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
        return get();
    }

    inline auto operator()() noexcept(!enable_exceptions_propagation) {
        return resume();
    }

public:
    /**
     * C++ boilerplate to disable copy assignement and copy constructor and leave just the move ones.
     */
    single_task() = default;

    constexpr explicit single_task(std::coroutine_handle<promise_type> handle) noexcept: handle_(handle) {}

    single_task(const single_task &) = delete;

    constexpr single_task(single_task &&other) noexcept: handle_(other.handle_) { other.handle_ = nullptr; }

    single_task &operator=(const single_task &) = delete;

    constexpr inline single_task &operator=(single_task &&other) noexcept {
        if (&other != this) {
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    ~single_task() noexcept {
        destroy();
    }


private:
    std::coroutine_handle<promise_type> handle_;
};

template<typename T, bool start_immediately, bool enable_exceptions_propagation>
struct single_task_promise_type : value_holder<T, enable_exceptions_propagation> {
    using single_task_t = single_task<T, start_immediately, enable_exceptions_propagation>;
    using value_holder_t = value_holder<T, enable_exceptions_propagation>;

public:
    /* Called by the compiler to convert the promise type into the return object, for the caller. */
    auto get_return_object() noexcept {
        return single_task_t(std::coroutine_handle<single_task_promise_type>::from_promise(*this)); // Compiler can figure out the offset to the handle
    }

    /* Whether the coroutine suspends itself before it starts executing */
    constexpr static auto initial_suspend() noexcept {
        if constexpr(start_immediately)
            return std::suspend_never{};
        else
            return std::suspend_always{};
    }

    /* Whether the coroutine suspends itself at the end before destruction. This is done to avoid the coroutine automatic destruction */
    constexpr static auto final_suspend() noexcept { return std::suspend_always{}; }

    /* When we return from the coroutine ; called from a co_return  */
    template<typename U = T>
    constexpr void return_value(U &&val) requires (!std::is_void_v<T>) { value_holder_t::set_value(std::forward<U>(val)); }

    template<typename U = T>
    constexpr void return_value(const U &val) requires (!std::is_void_v<T>) { value_holder_t::set_value(std::forward<U>(val)); }

    constexpr void unhandled_exception() noexcept {
        if constexpr (enable_exceptions_propagation) {
            value_holder_t::set_exception(std::current_exception());
        }
    }

};

template<bool start_immediately, bool enable_exceptions_propagation>
struct single_task_promise_type<void, start_immediately, enable_exceptions_propagation> : value_holder<void, enable_exceptions_propagation> {
    using single_task_t = single_task<void, start_immediately, enable_exceptions_propagation>;
    using value_holder_t = value_holder<void, enable_exceptions_propagation>;

public:
    /* Called by the compiler to convert the promise type into the return object, for the caller. */
    auto get_return_object() noexcept {
        return single_task_t(std::coroutine_handle<single_task_promise_type>::from_promise(*this)); // Compiler can figure out the offset to the handle
    }

    /* Whether the coroutine suspends itself before it starts executing */
    constexpr static auto initial_suspend() noexcept {
        if constexpr(start_immediately)
            return std::suspend_never{};
        else
            return std::suspend_always{};
    }

    /* Whether the coroutine suspends itself at the end before destruction. This is done to avoid the coroutine automatic destruction */
    constexpr static auto final_suspend() noexcept { return std::suspend_always{}; }

    constexpr void unhandled_exception() noexcept {
        if constexpr(enable_exceptions_propagation) {
            value_holder_t::set_exception(std::current_exception());
        }
    }

    /* We return nothing from the coroutine ; called from a co_return without argument or falling of the end of a coroutine */
    constexpr static void return_void() noexcept requires(std::is_void_v<void>) {}

};


static constexpr void static_tests_single_task() {
    static_assert(sizeof(single_task<int, false, false>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<int, false, true>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<int, true, false>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<int, true, true>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<void, false, false>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<void, false, true>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<void, true, false>) == sizeof(std::coroutine_handle<void>));
    static_assert(sizeof(single_task<void, true, true>) == sizeof(std::coroutine_handle<void>));

    static_assert(sizeof(single_task<void, false, false>::promise_type) == 1);
    static_assert(sizeof(single_task<void, true, false>::promise_type) == 1);
    static_assert(sizeof(single_task<int, false, false>::promise_type) == sizeof(int));
    static_assert(sizeof(single_task<int, true, false>::promise_type) == sizeof(int));
    static_assert(sizeof(single_task<void, false, true>::promise_type) == sizeof(std::exception_ptr));
    static_assert(sizeof(single_task<void, true, true>::promise_type) == sizeof(std::exception_ptr));
    static_assert(sizeof(single_task<int, false, true>::promise_type) == sizeof(std::variant<int, std::exception_ptr>));
    static_assert(sizeof(single_task<int, true, true>::promise_type) == sizeof(std::variant<int, std::exception_ptr>));
}

