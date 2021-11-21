#pragma once

#include <type_traits>
#include <variant>

struct empty_storage_struct {
};

template<typename true_type, typename false_type, bool b>
struct conditional_type;

template<typename true_type, typename false_type>
struct conditional_type<true_type, false_type, true> {
    using type = true_type;
};

template<typename true_type, typename false_type>
struct conditional_type<true_type, false_type, false> {
    using type = false_type;
};

template<typename true_type, typename false_type, bool b>
using conditional_type_t = typename conditional_type<true_type, false_type, b>::type;

template<typename T, bool b>
using optional_type_t = conditional_type_t<T, empty_storage_struct, b>;

constexpr void check_type_helpers() {
    static_assert(std::is_same_v<conditional_type_t<int, float, true>, int>);
    static_assert(std::is_same_v<conditional_type_t<int, float, false>, float>);

    static_assert(sizeof(optional_type_t<float, true>) == sizeof(float));
    static_assert(sizeof(optional_type_t<float, false>) == 1);
}

template<typename T, bool enable_exceptions_propagation>
struct value_holder {
public:
    template<typename U = void>
    inline constexpr std::enable_if_t<enable_exceptions_propagation, U> set_exception(const std::exception_ptr &ptr) noexcept {
        return_value_ = ptr;
    }

    template<typename U = T>
    inline constexpr void set_value(U &&val) noexcept {
        return_value_ = val;
    }

    template<typename U = T>
    inline constexpr void set_value(const U &val) noexcept {
        return_value_ = val;
    }

    inline std::exception_ptr get_exception_ptr() noexcept {
        if constexpr (enable_exceptions_propagation) {
            if (!std::holds_alternative<std::exception_ptr>(return_value_)) {
                return nullptr;
            }
            auto ptr = std::get<std::exception_ptr>(return_value_);
            return_value_ = std::exception_ptr{};
            return ptr;
        } else {
            return nullptr;
        }
    }

    constexpr T const &get_value() noexcept {
        if constexpr(enable_exceptions_propagation) {
            return std::get<T>(return_value_);
        } else {
            return return_value_;
        }
    }

private:
    using storage_t = conditional_type_t<std::variant<T, std::exception_ptr>, T, enable_exceptions_propagation>;
    storage_t return_value_;
};

template<>
struct value_holder<void, true> {
public:
    inline constexpr void set_exception(const std::exception_ptr &ptr) noexcept {
        return_value_ = ptr;
    }

    inline std::exception_ptr get_exception_ptr() noexcept {
        if (!return_value_) {
            return nullptr;
        }
        const auto ptr = return_value_;
        return_value_ = nullptr;
        return ptr;
    }

private:
    std::exception_ptr return_value_;
};

/**
 * Allow EBO
 */
template<>
struct value_holder<void, false> {
};
