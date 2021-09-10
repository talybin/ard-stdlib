#include "variant.hpp"
#include "Particle.h"

void ard::on_exception(const std::exception& ex) {
    // Log exception before reboot
    Particle.publish("Error", ex.what(), PRIVATE);
    System.reset();
}

void setup()
{
    std::variant<int, float> v;
    v = 19.5f; // v contains float

    bool success = std::visit([](auto temp) {
        return Particle.publish("temperature", String::format("%.1f", temp));
    }, v);
}

void loop() { }

