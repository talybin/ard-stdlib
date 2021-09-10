## ARD-STDLIB

Library of type traits and utilities from C++17/20/23 and experimental TS.

### Currently supporting

variant.hpp

* [variant](https://en.cppreference.com/w/cpp/utility/variant)

optional.hpp

* [optional](https://en.cppreference.com/w/cpp/utility/optional)

string_view.hpp

* [string_view](https://en.cppreference.com/w/cpp/string/basic_string_view)

memory.hpp

* [destroy_at](https://en.cppreference.com/w/cpp/memory/destroy_at)

utility.hpp

* [in_place_t, in_place_type_t, in_place_index_t](https://en.cppreference.com/w/cpp/utility/in_place)

type_traits.hpp

* [void_t](https://en.cppreference.com/w/cpp/types/void_t)
* [bool_constant](https://en.cppreference.com/w/cpp/types/integral_constant)
* [conjunction](https://en.cppreference.com/w/cpp/types/conjunction)
* [disjunction](https://en.cppreference.com/w/cpp/types/disjunction)
* [negation](https://en.cppreference.com/w/cpp/types/negation)
* [type_identity](https://en.cppreference.com/w/cpp/types/type_identity)
* [remove_cvref](https://en.cppreference.com/w/cpp/types/remove_cvref)
* [nonesuch](https://en.cppreference.com/w/cpp/experimental/nonesuch)
* [is_detected, detected_or, is_detected_exact, is_detected_convertible](https://en.cppreference.com/w/cpp/experimental/is_detected)

### Get started

All header files has the same name as original but with postfix `.hpp`. For example, to include variant use `variant.hpp`.

```cpp
#include "variant.hpp"

void setup()
{
    std::variant<int, float> v;
    v = 19.5f; // v contains float

    bool success = std::visit([](auto temp) {
        return Particle.publish("temperature", String::format("%.1f", temp));
    }, v);
}

void loop() { }
```

### Exception handling

As you may know, exceptions are disabled on Arduino. Instead of throwing exception, library will call special handler function with exception as argument. It is defined in `exception.hpp` and in case of an exception, will reboot the device.

To catch exception override exception handler.

```cpp
#include "exception.hpp"

void ard::on_exception(const std::exception& ex) {
    // Log exception before reboot
    Particle.publish("Error", ex.what(), PRIVATE);
}

void setup() { }
void loop() { }
```

**Note!** The exception handler is located in `ard` namespace.

