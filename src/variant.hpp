#pragma once
#if __cplusplus >= 201703L
#include <variant>
#else

#include "utility.hpp"
#include "memory.hpp"
#include "type_traits.hpp"
#include <stdexcept>

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
    struct variant_alternative;

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

    #if 0
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
    #endif

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

        // TODO fix below

        template <class U> // U is variadic_union<...>
        constexpr decltype(auto) get(std::in_place_index_t<0>, U&& u) {
            return std::forward<U>(u).first_.get();
        }

        template <size_t N, class U>
        constexpr decltype(auto) get(std::in_place_index_t<N>, U&& u) {
            return get(std::in_place_index_t<N-1>{}, std::forward<U>(u).rest_);
        }

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
        struct overload_set<First, Rest...> : overload_set<Rest...>
        {
            using overload_set<Rest...>::s_fun;
            static std::integral_constant<size_t, sizeof...(Rest)> s_fun(First);
        };

        // Skip default init
        template <class... Rest>
        struct overload_set<void, Rest...> : overload_set<Rest...>
        {
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

        template <class...>
        struct is_unique : std::true_type { };

        template <class T, class...Rest>
        struct is_unique<T, Rest...> {
            static constexpr bool value =
                !std::disjunction<std::is_same<T, Rest>...>::value && is_unique<Rest...>::value;
        };

    } // namespace variant_detail

    template <class... Ts>
    struct variant {
    private:
        static_assert(sizeof...(Ts) > 0,
            "variant must have at least one alternative");
        static_assert(!std::disjunction<std::is_reference<Ts>...>::value,
            "variant must have no reference alternative");
        static_assert(!std::disjunction<std::is_void<Ts>...>::value,
            "variant must have no void alternative");
        static_assert(variant_detail::is_unique<Ts...>::value,
            "variant must have different types");

        template <size_t... I>
        void reset_impl(std::index_sequence<I...>) {
            static constexpr void (*vtable[])(const variant&) = {
                &variant_detail::erased_dtor<const variant&, I>...
            };
            if (index_ != variant_npos)
                vtable[index_](*this);
        }

        void reset() {
            reset_impl(std::index_sequence_for<Ts...>());
            index_ = variant_npos;
        }

        template <class T>
        static constexpr size_t accepted_index =
            variant_detail::accepted_index<T, variant>::value;

        variant_detail::variadic_union<Ts...> union_;
        size_t index_ = variant_npos;

        template <size_t N, class Variant>
        friend constexpr decltype(auto) get(Variant&&);

    public:
        constexpr variant() : index_(variant_npos) { }

        template <class T,
            class = std::enable_if_t<not std::is_same<std::decay_t<T>, variant>::value> >
        constexpr variant(T&& t)
        : variant(std::in_place_index_t< accepted_index<T> >{}, std::forward<T>(t))
        { }

        template <size_t N, class... Args>
        constexpr variant(std::in_place_index_t<N>, Args&&... args)
        : union_(std::in_place_index_t<N>{}, std::forward<Args>(args)...)
        , index_(N)
        { }

        ~variant() {
            reset();
        }

        template <class T>
        std::enable_if_t<not std::is_same<std::decay_t<T>, variant>::value, variant&>
        operator=(T&& t) {
            constexpr auto idx = accepted_index<T>;
            if (index_ == idx)
                get<idx>(*this) = std::forward<T>(t);
            else
                this->emplace<idx>(std::forward<T>(t));
            return *this;
        }

        // https://en.cppreference.com/w/cpp/utility/variant/emplace
        // 1
        template <class T, class... Args>
        T& emplace(Args&&... args) {
            return this->emplace<accepted_index<T>>(std::forward<Args>(args)...);
        }
        // 3
        template <size_t N, class... Args>
        variant_alternative_t<N, variant>& emplace(Args&&... args) {
            static_assert(N < sizeof...(Ts),
                "The index should be in [0, number of alternatives)");
            this->~variant();
            ::new (this) variant(std::in_place_index_t<N>{}, std::forward<Args>(args)...);
            return get<N>(*this);
        }

        // https://en.cppreference.com/w/cpp/utility/variant/index
        size_t index() const noexcept {
            return index_;
        }

        // Returns false if and only if the variant holds a value
        constexpr bool valueless_by_exception() const noexcept {
            return index_ == variant_npos;
        }
    };

    template <size_t N, class Variant>
    constexpr decltype(auto) get(Variant&& v) {
        return get(std::in_place_index_t<N>{}, std::forward<Variant>(v).union_);
    }

    #if 0
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
    #endif

    // Should inherit from std::exception, but for shorter version...
    using bad_variant_access = std::runtime_error;

    // Visitor details
    namespace detail
    {
        template <size_t I, class Visitor, class Variant>
        inline void visit_invoke(Visitor&& visitor, Variant&& var) {
            std::forward<Visitor>(visitor)(get<I>(std::forward<Variant>(var)));
        }

    } // namespace detail

    // Visitor implementation
    template <class Visitor, class Variant, size_t... I>
    constexpr void visit(Visitor&& visitor, Variant&& var, std::index_sequence<I...>)
    {
        if (var.valueless_by_exception())
            throw bad_variant_access("Unexpected index");

        constexpr void (*vtable[])(Visitor&& visitor, Variant&& var) = {
            &detail::visit_invoke<I, Visitor, Variant>...
        };

        vtable[var.index()](
            std::forward<Visitor>(visitor), std::forward<Variant>(var));
    }

    // Simplified visitor. Actual implementation takes a list of variants
    // and can return a value.
    template <class Visitor, class Variant>
    constexpr void visit(Visitor&& visitor, Variant&& var)
    {
        using T = std::remove_reference_t<Variant>;
        visit(
            std::forward<Visitor>(visitor),
            std::forward<Variant>(var),
            std::make_index_sequence<variant_size<T>::value>());
    }

} // namespace std

#endif // __cplusplus < 201703L

