#include "variant.hpp"

void setup()
{
    std::variant<int, float> v;
    v = 19.5; // v contains float

    bool success = std::visit([](auto temp) {
        return Particle.publish("temperature", String::format("%.1f", temp));
    }, v);
}

void loop() { }

