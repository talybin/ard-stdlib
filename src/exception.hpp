// Arduino exception handling
// Vladimir Talybin (2021)
//
// File version: 1.0.0

#pragma once

#include <exception>
#include <string>

namespace ard
{
    // Define this function to catch exception
    extern void on_exception(const std::exception&) __attribute__((weak));

    // Streaming exception
    struct error : std::exception {
    private:
        std::string err_;

    public:
        error() = default;

        error(std::string err)
        : err_(std::move(err))
        { }

        const char* what() const noexcept override
        { return err_.c_str(); }

        template <class T>
        auto operator<<(T arg) -> decltype(std::to_string(arg), *this) {
            err_.append(std::to_string(arg));
            return *this;
        }

        template <class T>
        auto operator<<(const T& arg) -> decltype(std::string().append(arg), *this) {
            err_.append(arg);
            return *this;
        }

        error& operator<<(char arg) {
            err_.append(1, arg);
            return *this;
        }
    };

    // Call throw_error instead of throw
    [[noreturn]] inline void
    throw_exception(const std::exception& err) {
        if (on_exception)
            on_exception(err);
        System.reset();
    }

} // namespace ard

