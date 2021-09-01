#pragma once
#include <algorithm>

#include "memory.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0323r4.html

namespace std
{
    template <class E>
    struct not_expected
    {
        not_expected() = delete;
        constexpr explicit not_expected(const E&) {}
        constexpr explicit not_expected(E&&) {}
        constexpr const E& value() const & {}
        constexpr E& value() & { return value_; }
        constexpr E&& value() && { return value_; }
        constexpr E const&& value() const && { return value_; }

    private:
        E value_;
    };

    // unexpect tag
    struct unexpect_t {
        explicit unexpect_t() = default;
    };

    // bad_expected_access
    template <class E>
    struct bad_expected_access;

    // Specialization for void
    template <>
    struct bad_expected_access<void>;

    // Expected for object types
    template <class Value, class Error>
    struct expected {
    public:
        static_assert(!std::is_same<Value, void>::value, "void type not implemented yet");

        using value_type = Value;
        using error_type = Error;

        using unexpected_type = not_expected<error_type>;

    private:
        // False means contains unexpected_type
        bool has_value_ = false;
        // Union storage
        unsigned char storage_[std::max({ sizeof(value_type), sizeof(unexpected_type) })];

        // Alias
        template <class T>
        using not_self = std::negation<std::is_same<std::decay_t<T>, expected>>;

        // Create type T without destroying storage
        template <class T, class... Args>
        void create(Args&&... args)
        {
            ::new (storage_) T(std::forward<Args>(args)...);
            has_value_ = std::is_same<T, value_type>::value;
        }

        // Destroy storage
        void destroy()
        {
            if (has_value_) {
                std::destroy_at((value_type*)storage_);
                has_value_ = false;
            }
            else
                std::destroy_at((unexpected_type*)storage_);
        }

    public:
        // constructors

        constexpr expected()
        { create<value_type>(); }

        constexpr expected(const expected& rhs)
        { rhs.has_value_ ? create<value_type>(rhs.value()) : create<unexpected_type>(rhs.error()); }

        constexpr expected(expected&& rhs)
        { rhs.has_value_ ? create<value_type>(std::move(rhs).value()) : create<unexpected_type>(std::move(rhs).error()); }

        template <class T, class E,
            class = std::enable_if_t<std::is_constructible<value_type, const T&>::value>,
            class = std::enable_if_t<std::is_constructible<error_type, const E&>::value>
        >
        constexpr explicit expected(const expected<T, E>& rhs)
        { rhs.has_value_ ? create<value_type>(rhs.value()) : create<unexpected_type>(rhs.error()); }

        template <class T, class E,
            class = std::enable_if_t<std::is_constructible<value_type, T&&>::value>,
            class = std::enable_if_t<std::is_constructible<error_type, E&&>::value>
        >
        constexpr explicit expected(expected<T, E>&& rhs)
        { rhs.has_value_ ? create<value_type>(std::move(rhs).value()) : create<unexpected_type>(std::move(rhs).error()); }

        template <class T = value_type,
            class = std::enable_if_t<std::is_constructible<value_type, T>::value>,
            class = std::enable_if_t<not_self<T>::value>
        >
        constexpr explicit expected(T&& v)
        { create<value_type>(std::forward<T>(v)); }

        template <class... Args,
            class = std::enable_if_t<std::is_constructible<value_type, Args...>::value>
        >
        constexpr explicit expected(std::in_place_t, Args&&... args)
        { create<value_type>(std::forward<Args>(args)...); }

        template <class T, class... Args,
            class = std::enable_if_t<std::is_constructible<value_type, std::initializer_list<T>&, Args...>::value>
        >
        constexpr explicit expected(std::in_place_t, std::initializer_list<T> il, Args&&... args)
        { create<value_type>(il, std::forward<Args>(args)...); }

        template <class E = error_type>
        constexpr expected(const not_expected<E>& u)
        { create<unexpected_type>(u); }

        template <class E = error_type>
        constexpr expected(not_expected<E>&& u)
        { create<unexpected_type>(std::move(u)); }

        template <class... Args,
            class = std::enable_if_t<std::is_constructible<unexpected_type, Args...>::value>
        >
        constexpr explicit expected(unexpect_t, Args&&... args)
        { create<unexpected_type>(std::forward<Args>(args)...); }

        template <class T, class... Args,
            class = std::enable_if_t<std::is_constructible<unexpected_type, std::initializer_list<T>&, Args...>::value>
        >
        constexpr explicit expected(unexpect_t, std::initializer_list<T> il, Args&&... args)
        { create<unexpected_type>(il, std::forward<Args>(args)...); }

        // destructor

        ~expected() { destroy(); }

        // assignment

        expected& operator=(const expected& rhs)
        {
            if (has_value_) {
                if (rhs.has_value_)
                    // Both contain values
                    value() = rhs.value();
                else {
                    // Self is value and rhs is error
                    destroy();
                    create<unexpected_type>(rhs.error());
                }
            }
            else if (rhs.has_value_) {
                // Self is error and rhs is value
                destroy();
                create<value_type>(rhs.value());
            }
            else
                // Both contain errors
                error() = rhs.error();
            return *this;
        }

        expected& operator=(expected&& rhs)
        {
            // Same logic as above
            if (has_value_) {
                if (rhs.has_value_)
                    // Both contain values
                    value() = std::move(rhs).value();
                else {
                    // Self is value and rhs is error
                    destroy();
                    create<unexpected_type>(std::move(rhs).error());
                }
            }
            else if (rhs.has_value_) {
                // Self is error and rhs is value
                destroy();
                create<value_type>(std::move(rhs).value());
            }
            else
                // Both contain errors
                error() = std::move(rhs).error();
            return *this;
        }

        template <class T = value_type,
            class = std::enable_if_t<not_self<T>::value>
        >
        expected& operator=(T&& v)
        {
            if (has_value_)
                value() = std::forward<T>(v);
            else {
                // Destroy not_expected and create value
                destroy();
                create<value_type>(std::forward<T>(v));
            }
            return *this;
        }

        template <class E = error_type>
        expected& operator=(const not_expected<E>& u)
        {
            if (has_value_) {
                // Destroy value and assign error
                destroy();
                create<unexpected_type>(u);
            }
            else
                error() = u.value();
            return *this;
        }

        template <class E = error_type>
        expected& operator=(not_expected<E>&& u)
        {
            if (has_value_) {
                // Destroy value and assign error
                destroy();
                create<unexpected_type>(std::move(u));
            }
            else
                error() = std::move(u).error();
            return *this;
        }

        // emplace

        template <class... Args>
        value_type& emplace(Args&&... args)
        { return (destroy(), create<value_type>(std::forward<Args>(args)...), value()); }

        template <class T, class... Args>
        value_type& emplace(initializer_list<T> il, Args&&... args)
        { return (destroy(), create<value_type>(il, std::forward<Args>(args)...), value()); }

        // swap

        void swap(expected& rhs) noexcept
        {
            std::swap(storage_, rhs.storage_);
            std::swap(has_value_, rhs.has_value_);
        }

        // observers

        constexpr const value_type* operator->() const noexcept
        { return (value_type*)storage_; }

        constexpr value_type* operator->() noexcept
        { return (value_type*)storage_; }

        constexpr const value_type& operator*() const & noexcept
        { return value(); }

        constexpr value_type& operator*() & noexcept
        { return value(); }

        constexpr const value_type&& operator*() const && noexcept
        { return value(); }

        constexpr value_type&& operator*() && noexcept
        { return value(); }

        constexpr explicit operator bool() const noexcept
        { return has_value_; }

        constexpr bool has_value() const noexcept
        { return has_value_; }

        constexpr const value_type& value() const &
        {
            if (has_value_)
                return *(value_type*)storage_;
            // TODO call exception_handler (should be weak and by default abort)
            // void exception_handler() __attribute__ ((__noreturn__));
            //throw bad_expected_access();
        }

        constexpr value_type& value() & noexcept
        { return *(value_type*)storage_; }

        constexpr const value_type&& value() const && noexcept
        { return std::move(*(value_type*)storage_); }

        constexpr value_type&& value() && noexcept
        { return std::move(*(value_type*)storage_); }

        constexpr const error_type& error() const & noexcept
        { return ((unexpected_type*)storage_)->value(); }

        constexpr error_type& error() & noexcept
        { return ((unexpected_type*)storage_)->value(); }

        constexpr const error_type&& error() const && noexcept
        { return std::move(*(unexpected_type*)storage_).value(); }

        constexpr error_type&& error() && noexcept
        { return std::move(*(unexpected_type*)storage_).value(); }

        template <class T,
            class = std::enable_if_t<std::is_constructible<value_type, T>::value>
        >
        constexpr value_type value_or(T&& v) const & noexcept
        { return has_value_ ? value() : std::forward<T>(v); }

        template <class T,
            class = std::enable_if_t<std::is_constructible<value_type, T>::value>
        >
        value_type value_or(T&& v) && noexcept
        { return has_value_ ? std::move(value()) : std::forward<T>(v); }
    };

} // namespace std

