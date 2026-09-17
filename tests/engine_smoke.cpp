#include "Engine.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr std::size_t kStride = 8;

[[noreturn]] void fail(const char* message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void expect(bool condition, const char* message) {
    if (!condition) {
        fail(message);
    }
}

} // namespace

int main() {
    cc::Engine engine;

    const std::vector<float> initial = engine.unitSnapshot();
    expect(initial.size() == 12 * kStride, "expected 12 bootstrap units");
    expect(initial[4] == 1.0f, "first friendly should start selected");
    expect(std::fabs(initial[1] - 180.0f) < 0.01f, "unexpected first-unit x");
    expect(std::fabs(initial[2] - 250.0f) < 0.01f, "unexpected first-unit y");

    // The direct line crosses the first building, so reaching this destination
    // requires obstacle-aware routing rather than straight-line movement.
    engine.tap(900.0f, 250.0f);
    for (int i = 0; i < 1200; ++i) {
        engine.step(1.0f / 60.0f);
    }

    const std::vector<float> moved = engine.unitSnapshot();
    expect(std::fabs(moved[1] - 900.0f) < 8.0f, "selected unit failed to reach ordered x");
    expect(std::fabs(moved[2] - 250.0f) < 8.0f, "selected unit failed to reach ordered y");

    engine.reset();
    const std::vector<float> reset = engine.unitSnapshot();
    expect(std::fabs(reset[1] - initial[1]) < 0.01f, "reset did not restore x");
    expect(std::fabs(reset[2] - initial[2]) < 0.01f, "reset did not restore y");

    std::cout << "engine_smoke: PASS\n";
    return 0;
}
