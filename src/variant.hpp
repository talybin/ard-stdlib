#pragma once
#if __cplusplus >= 201703L
#include <variant>
#else

#include "utility.hpp"
#include "memory.hpp"
#include "type_traits.hpp"
#include "functional.hpp"
#include <exception>

namespace std
{
    template <class...>
    struct variant;

    // variant_size
    template <class>
    struct variant_size;

    template <class Variant>
    struct variant_size<const Variant> : variant_size<Variant> {};

    template <class Variant>
    struct variant_size<volatile Variant> : variant_size<Variant> {};

    template <class Variant>
    struct variant_size<const volatile Variant> : variant_size<Variant> {};

    template <class... Ts>
    struct variant_size<variant<Ts...>>
    : std::integral_constant<size_t, sizeof...(Ts)> {};

    // variant_alternative
    template <size_t N, class Variant>
    struct variant_alternative {
        static_assert(N < variant_size<Variant>::value,
            "The index must be in [0, number of alternatives)");
    };

    template <size_t N, class First, class... Rest>
    struct variant_alternative<N, variant<First, Rest...>>
    : variant_alternative<N - 1, variant<Rest...>> {};

    template <class First, class... Rest>
    struct variant_alternative<0, variant<First, Rest...>> {
        using type = First;
    };

    template <size_t N, class Variant>
    using variant_alternative_t = typename variant_alternative<N, Variant>::type;

    template <size_t N, class Variant>
    struct variant_alternative<N, const Variant> {
        using type = std::add_const_t<variant_alternative_t<N, Variant>>;
    };

    template <size_t N, class Variant>
    struct variant_alternative<N, volatile Variant> {
        using type = std::add_volatile_t<variant_alternative_t<N, Variant>>;
    };

    template <size_t N, class Variant>
    struct variant_alternative<N, const volatile Variant> {
        using type = std::add_cv_t<variant_alternative_t<N, Variant>>;
    };

    // variant_npos
    enum : size_t { variant_npos = size_t(-1) };

    // get
    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>>&
    get(variant<Ts...>&);

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>>&&
    get(variant<Ts...>&&);

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>> const&
    get(const variant<Ts...>&);

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>> const&&
    get(const variant<Ts...>&&);

    // detail
    namespace variant_detail
    {
        // Returns the first appearence of T in Ts.
        // Returns sizeof...(Ts) if T is not in Ts.
        template <class T, class... Ts>
        struct index_of : std::integral_constant<size_t, 0> {};

        template <class T, class First, class... Rest>
        struct index_of<T, First, Rest...> : std::integral_constant<
            size_t,
            std::is_same<T, First>::value ? 0 : index_of<T, Rest...>::value + 1> {};

        // How many times does T appear in type sequence?
        template <class T, class... Ts>
        struct type_count : std::integral_constant<size_t, 0> {};

        template <class T, class First, class... Rest>
        struct type_count<T, First, Rest...> : std::integral_constant<
            size_t,
            type_count<T, Rest...>::value + std::is_same<T, First>::value> {};

        // Check T appear exactly once in type list
        template <class T, class... Ts>
        struct exactly_once : std::bool_constant<type_count<T, Ts...>::value == 1> {};

        // Check all types appear exactly once in type list
        template <class...>
        struct is_unique : std::true_type { };

        template <class T, class...Rest>
        struct is_unique<T, Rest...> : std::bool_constant<
            !std::disjunction<std::is_same<T, Rest>...>::value &&
            is_unique<Rest...>::value> {};

        // Chack T is in_place_type_t or in_place_index_t
        template <class T>
        struct is_in_place_tag : std::false_type {};

        template <class T>
        struct is_in_place_tag<std::in_place_type_t<T>> : std::true_type {};

        template <size_t N>
        struct is_in_place_tag<std::in_place_index_t<N>> : std::true_type {};

        // std::visit invoker
        template <size_t I, class Visitor, class Variant>
        inline decltype(auto) visit_invoke(Visitor&& visitor, Variant&& var) {
            return std::forward<Visitor>(visitor)(get<I>(std::forward<Variant>(var)));
        }

        // uninitialized<T> is guaranteed to be a trivially destructible type,
        // even if T is not
        template <class T, bool = std::is_trivially_destructible<T>::value>
        struct uninitialized;

        template <class T>
        struct uninitialized<T, true>
        {
            template <class... Args>
            constexpr uninitialized(std::in_place_t, Args&&... args)
            : storage_(std::forward<Args>(args)...)
            { }

            constexpr const T& get() const & noexcept { return storage_; }
            constexpr T& get() & noexcept { return storage_; }

            constexpr const T&& get() const && noexcept { return std::move(storage_); }
            constexpr T&& get() && noexcept { return std::move(storage_); }

            T storage_;
        };

        template <class T>
        struct uninitialized<T, false>
        {
            template <class... Args>
            constexpr uninitialized(std::in_place_t, Args&&... args) {
                ::new (storage_) T(std::forward<Args>(args)...);
            }

            const T& get() const & noexcept { return *(T*)storage_; }
            T& get() & noexcept { return *(T*)storage_; }

            const T&& get() const && noexcept { return std::move(*(T*)storage_); }
            T&& get() && noexcept { return std::move(*(T*)storage_); }

            unsigned char storage_[sizeof(T)];
        };

        // Defines members and ctors
        template <class...>
        union variadic_union {};

        template <class First, class... Rest>
        union variadic_union<First, Rest...>
        {
            constexpr variadic_union() : rest_() { }

            template <class... Args>
            constexpr variadic_union(std::in_place_index_t<0>, Args&&...args)
            : first_(std::in_place_t{}, std::forward<Args>(args)...)
            { }

            template <size_t N, class... Args>
            constexpr variadic_union(std::in_place_index_t<N>, Args&&...args)
            : rest_(std::in_place_index_t<N - 1>{}, std::forward<Args>(args)...)
            { }

            uninitialized<First> first_;
            variadic_union<Rest...> rest_;
        };

        // variadic_union get
        template <class Union>
        constexpr decltype(auto) get(std::in_place_index_t<0>, Union&& u) noexcept {
            return std::forward<Union>(u).first_.get();
        }

        template <size_t N, class Union>
        constexpr decltype(auto) get(std::in_place_index_t<N>, Union&& u) noexcept {
            return variant_detail::get(
                std::in_place_index_t<N - 1>{}, std::forward<Union>(u).rest_);
        }

        // variant get
        template <size_t N, class Variant>
        constexpr decltype(auto) get(Variant&& v) noexcept {
            return variant_detail::get(
                std::in_place_index_t<N>{}, std::forward<Variant>(v).union_);
        }

        // Destruct value
        template <class T, bool = std::is_trivially_destructible<std::decay_t<T>>::value>
        struct value_dtor {
            static void destroy(T&&) {}
        };

        template <class T>
        struct value_dtor<T, false> {
            static void destroy(T&& v) {
                std::destroy_at(std::addressof(v));
            }
        };

        // erased_dtor
        template <class Variant, size_t N>
        void erased_dtor(Variant&& v) {
            value_dtor<decltype(get<N>(v))>::destroy(get<N>(v));
        }

        // Takes Ts and create an indexed overloaded s_fun for each type.
        // If a type appears more than once in Ts, create only one overload.
        template <class... Ts>
        struct overload_set {
            static void s_fun();
        };

        template <class First, class... Rest>
        struct overload_set<First, Rest...> : overload_set<Rest...> {
            using overload_set<Rest...>::s_fun;
            static std::integral_constant<size_t, sizeof...(Rest)> s_fun(First);
        };

        // Skip default init
        template <class... Rest>
        struct overload_set<void, Rest...> : overload_set<Rest...> {
            using overload_set<Rest...>::s_fun;
        };

        // accepted_index
        template <class T, class Variant, class = void>
        struct accepted_index {
            static constexpr size_t value = variant_npos;
        };

        // Find index to overload that T is assignable to.
        template <class T, class... Ts>
        struct accepted_index<T, variant<Ts...>,
            std::void_t<decltype(overload_set<Ts...>::s_fun(std::declval<T>()))> >
        {
            static constexpr size_t value = sizeof...(Ts) - 1 -
                decltype(overload_set<Ts...>::s_fun(std::declval<T>()))::value;
        };

        // variant_base
        template <class... Ts>
        struct variant_base {
        private:
            template <size_t... I>
            void reset_impl(std::index_sequence<I...>) {
                static constexpr void (*vtable[])(const variant<Ts...>&) = {
                    &variant_detail::erased_dtor<const variant<Ts...>&, I>...
                };
                if (index_ != variant_npos)
                    vtable[index_](static_cast<const variant<Ts...>&>(*this));
            }

        protected:
            constexpr variant_base() : index_(variant_npos) {}

            template <size_t N, class... Args>
            constexpr variant_base(std::in_place_index_t<N>, Args&&... args)
            : union_(std::in_place_index_t<N>{}, std::forward<Args>(args)...)
            , index_(N)
            {}

            ~variant_base() {
                reset();
            }

            void reset() {
                reset_impl(std::index_sequence_for<Ts...>());
                index_ = variant_npos;
            }

            variant_detail::variadic_union<Ts...> union_;
            size_t index_;
        };

    } // namespace variant_detail

    // holds_alternative
    template <class T, class... Ts>
    constexpr bool holds_alternative(const variant<Ts...>& v) noexcept {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        return v.index() == variant_detail::index_of<T, Ts...>::value;
    }

    // get
    template <class T, class... Ts>
    constexpr T& get(variant<Ts...>& v) {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get<variant_detail::index_of<T, Ts...>>(v);
    }

    template <class T, class... Ts>
    constexpr T&& get(variant<Ts...>&& v) {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get<variant_detail::index_of<T, Ts...>>(std::move(v));
    }

    template <class T, class... Ts>
    constexpr const T& get(const variant<Ts...>& v) {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get<variant_detail::index_of<T, Ts...>>(v);
    }

    template <class T, class... Ts>
    constexpr const T&& get(const variant<Ts...>&& v) {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get<variant_detail::index_of<T, Ts...>>(std::move(v));
    }

    // get_if
    template <size_t N, class... Ts>
    constexpr std::add_pointer_t<variant_alternative_t<N, variant<Ts...>>>
    get_if(variant<Ts...>* ptr) noexcept {
        using T = variant_alternative_t<N, variant<Ts...>>;
        static_assert(N < sizeof...(Ts),
            "The index must be in [0, number of alternatives)");
        if (ptr && ptr->index() == N)
            return std::addressof(variant_detail::get<N>(*ptr));
        return nullptr;
    }

    template <size_t N, class... Ts>
    constexpr std::add_pointer_t<const variant_alternative_t<N, variant<Ts...>>>
    get_if(const variant<Ts...>* ptr) noexcept { 
        using T = variant_alternative_t<N, variant<Ts...>>;
        static_assert(N < sizeof...(Ts),
            "The index must be in [0, number of alternatives)");
        if (ptr && ptr->index() == N)
            return std::addressof(variant_detail::get<N>(*ptr));
        return nullptr;
    }

    template <class T, class... Ts>
    constexpr std::add_pointer_t<T>
    get_if(variant<Ts...>* ptr) noexcept {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get_if<variant_detail::index_of<T, Ts...>>(ptr);
    }

    template <class T, class... Ts>
    constexpr std::add_pointer_t<const T>
    get_if(const variant<Ts...>* ptr) noexcept {
        static_assert(variant_detail::exactly_once<T, Ts...>::value,
            "T must occur exactly once in alternatives");
        static_assert(!std::is_void<T>::value, "T must not be void");
        return get_if<variant_detail::index_of<T, Ts...>>(ptr);
    }

    // monostate
    struct monostate {};

    // bad_variant_access
    struct bad_variant_access : std::exception
    {
        bad_variant_access() noexcept {}
        const char* what() const noexcept override { return reason_; }

    private:
        bad_variant_access(const char* reason) noexcept : reason_(reason) {}
        const char* reason_ = "bad variant access";
        friend void throw_bad_variant_access(const char* what);
    };

    // throw_bad_variant_access
    inline void throw_bad_variant_access(const char* what) {
        throw std::bad_variant_access(what);
    }

    inline void throw_bad_variant_access(bool valueless) {
        if (valueless)
            throw_bad_variant_access("std::get: variant is valueless");
        else
            throw_bad_variant_access("std::get: wrong index for variant");
    }

    // variant
    template <class... Ts>
    struct variant : variant_detail::variant_base<Ts...> {
    private:
        static_assert(sizeof...(Ts) > 0,
            "variant must have at least one alternative");
        static_assert(!std::disjunction<std::is_reference<Ts>...>::value,
            "variant must have no reference alternative");
        static_assert(!std::disjunction<std::is_void<Ts>...>::value,
            "variant must have no void alternative");
        static_assert(variant_detail::is_unique<Ts...>::value,
            "variant must have different types");

        using base = variant_detail::variant_base<Ts...>;

        template <class T>
        using not_self = std::negation<std::is_same<std::decay_t<T>, variant>>;

        template <size_t N, class = std::enable_if_t<(N < sizeof...(Ts))>>
        using to_type = variant_alternative_t<N, variant>;

        template <class T>
        static constexpr size_t accepted_index =
            variant_detail::accepted_index<T, variant>::value;

        template <class T, class = std::enable_if_t<not_self<T>::value>>
        using accepted_type = to_type<accepted_index<T>>;

        template <class T>
        static constexpr bool not_in_place_tag =
            !variant_detail::is_in_place_tag<std::decay_t<T>>::value;

        template <class T>
        static constexpr bool exactly_once =
            variant_detail::exactly_once<T, Ts...>::value;

        template <class T>
        static constexpr size_t index_of = variant_detail::index_of<T, Ts...>::value;

        // Getter
        template <size_t N, class Variant>
        friend constexpr decltype(auto) variant_detail::get(Variant&&) noexcept;

    public:
        variant() = default;
        variant(const variant& rhs) = default;
        variant(variant&&) = default;
        variant& operator=(const variant&) = default;
        variant& operator=(variant&&) = default;
        ~variant() = default;

        // Counstructors
        // https://en.cppreference.com/w/cpp/utility/variant/variant

        // 4
        template <class T,
            class = std::enable_if_t<not_in_place_tag<T>>,
            class U = accepted_type<T&&>,
            class = std::enable_if_t<
                exactly_once<U> && std::is_constructible<U, T>::value>
        >
        constexpr
        variant(T&& t)
        noexcept(std::is_nothrow_constructible<U, T>::value)
        : variant(std::in_place_index_t<accepted_index<T>>{}, std::forward<T>(t))
        {}

        // 5
        template <class T, class... Args,
            class = std::enable_if_t<
                exactly_once<T> && std::is_constructible<T, Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_type_t<T>, Args&&... args)
        : variant(std::in_place_index_t<index_of<T>>{}, std::forward<Args>(args)...)
        {}

        // 6
        template <class T, class U, class... Args,
            class = std::enable_if_t<
                exactly_once<T> &&
                std::is_constructible<T, std::initializer_list<U>&, Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
        : variant(std::in_place_index_t<index_of<T>>{}, il, std::forward<Args>(args)...)
        {}

        // 7
        template <size_t N, class... Args,
            class T = to_type<N>,
            class = std::enable_if_t<std::is_constructible<T, Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_index_t<N>, Args&&... args)
        : base(std::in_place_index_t<N>{}, std::forward<Args>(args)...)
        {}

        // 8
        template <size_t N, class U, class... Args,
            class T = to_type<N>,
            class = std::enable_if_t<
                std::is_constructible<T, std::initializer_list<U>&, Args...>::value>
        >
        constexpr explicit
        variant(std::in_place_index_t<N>, std::initializer_list<U> il, Args&&... args)
        : base(std::in_place_index_t<N>{}, il, std::forward<Args>(args)...)
        {}

        // Assign operators
        // https://en.cppreference.com/w/cpp/utility/variant/operator%3D

        // 3
        template <class T, class U = accepted_type<T&&>>
        std::enable_if_t<
            exactly_once<U> &&
            std::is_constructible<U, T>::value &&
            std::is_assignable<U&, T>::value,
            variant&
        >
        operator=(T&& rhs) noexcept(
            std::is_nothrow_assignable<U&, T>::value &&
            std::is_nothrow_constructible<U, T>::value)
        {
            constexpr auto idx = accepted_index<T>;
            if (index() == idx)
                get<idx>(*this) = std::forward<T>(rhs);
            else
                this->emplace<idx>(std::forward<T>(rhs));
            return *this;
        }

        // emplace
        // https://en.cppreference.com/w/cpp/utility/variant/emplace

        // 1
        template <class T, class... Args>
        std::enable_if_t<
            std::is_constructible<T, Args...>::value && exactly_once<T>,
            T&>
        emplace(Args&&... args) {
            return this->emplace<index_of<T>>(std::forward<Args>(args)...);
        }

        // 2
        template <class T, class U, class... Args>
        std::enable_if_t<
            std::is_constructible<T, std::initializer_list<U>&, Args...>::value &&
            exactly_once<T>,
            T&>
        emplace(std::initializer_list<U> il, Args&&... args) {
            return this->emplace<index_of<T>>(il, std::forward<Args>(args)...);
        }

        // 3
        template <size_t N, class... Args>
        std::enable_if_t<
            std::is_constructible<variant_alternative_t<N, variant>, Args...>::value,
            variant_alternative_t<N, variant>&>
        emplace(Args&&... args) {
            // Provide the strong exception-safety guarantee when possible,
            // to avoid becoming valueless.
            // This construction might throw.
            variant tmp(std::in_place_index_t<N>{}, std::forward<Args>(args)...);
            // But this step won't.
            *this = std::move(tmp);
            return std::get<N>(*this);
        }

        // 4
        template <size_t N, class U, class... Args>
        std::enable_if_t<
            std::is_constructible<
                variant_alternative_t<N, variant>, std::initializer_list<U>&, Args...
            >::value,
            variant_alternative_t<N, variant>&>
        emplace(std::initializer_list<U> il, Args&&... args) {
            return this->emplace<N>(il, std::forward<Args>(args)...);
        }

        // Returns the zero-based index of the alternative held by the variant
        constexpr size_t index() const noexcept {
            return this->index_;
        }

        // Returns false if and only if the variant holds a value
        constexpr bool valueless_by_exception() const noexcept {
            return this->index_ == variant_npos;
        }
    };

    // get
    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>>&
    get(variant<Ts...>& v) {
        if (v.index() != N)
            throw_bad_variant_access(v.valueless_by_exception());
        return variant_detail::get<N>(v);
    }

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>>&&
    get(variant<Ts...>&& v) {
        if (v.index() != N)
            throw_bad_variant_access(v.valueless_by_exception());
        return variant_detail::get<N>(std::move(v));
    }

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>> const&
    get(const variant<Ts...>& v) {
        if (v.index() != N)
            throw_bad_variant_access(v.valueless_by_exception());
        return variant_detail::get<N>(v);
    }

    template <size_t N, class... Ts>
    constexpr variant_alternative_t<N, variant<Ts...>> const&&
    get(const variant<Ts...>&& v) {
        if (v.index() != N)
            throw_bad_variant_access(v.valueless_by_exception());
        return variant_detail::get<N>(std::move(v));
    }

    // Visitor implementation
    template <class Visitor, class Variant, size_t... I>
    constexpr decltype(auto)
    visit(Visitor&& visitor, Variant&& var, std::index_sequence<I...>)
    {
        if (var.valueless_by_exception())
            throw_bad_variant_access("std::visit: variant is valueless");

        using R = std::invoke_result_t<
            Visitor, decltype(std::get<0>(std::declval<Variant>()))>;

        constexpr R (*vtable[])(Visitor&& visitor, Variant&& var) = {
            &variant_detail::visit_invoke<I, Visitor, Variant>...
        };

        return vtable[var.index()](
            std::forward<Visitor>(visitor), std::forward<Variant>(var));
    }

    // Support only one variant for now
    template <class Visitor, class Variant>
    constexpr decltype(auto)
    visit(Visitor&& visitor, Variant&& var)
    {
        using T = std::remove_reference_t<Variant>;
        return visit(
            std::forward<Visitor>(visitor),
            std::forward<Variant>(var),
            std::make_index_sequence<variant_size<T>::value>());
    }

} // namespace std

#endif // __cplusplus < 201703L

