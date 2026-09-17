#include "Engine.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

constexpr std::size_t kUnitStride = 12;
constexpr std::size_t kSoldierStride = 7;

[[noreturn]] void fail(const char* message) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

void expect(bool condition, const char* message) {
    if (!condition) {
        fail(message);
    }
}

float unitAmmo(const std::vector<float>& units, std::size_t unitIndex) {
    return units[unitIndex * kUnitStride + 11];
}

} // namespace

int main() {
    cc::Engine engine;

    const std::vector<float> initial = engine.unitSnapshot();
    const std::vector<float> soldiers = engine.soldierSnapshot();
    const std::vector<float> cover = engine.coverSnapshot();

    expect(initial.size() == 12 * kUnitStride, "expected 12 bootstrap squads");
    expect(soldiers.size() == 60 * kSoldierStride, "expected 60 bootstrap soldiers");
    expect(!cover.empty(), "expected bootstrap cover zones");
    expect(initial[4] == 1.0f, "first friendly should start selected");
    expect(initial[9] == 5.0f && initial[10] == 5.0f, "first squad should start with five living soldiers");
    expect(std::fabs(initial[1] - 180.0f) < 0.01f, "unexpected first-unit x");
    expect(std::fabs(initial[2] - 250.0f) < 0.01f, "unexpected first-unit y");

    engine.tap(900.0f, 250.0f);
    for (int i = 0; i < 1200; ++i) {
        engine.step(1.0f / 60.0f);
    }

    const std::vector<float> moved = engine.unitSnapshot();
    expect(std::fabs(moved[1] - 900.0f) < 8.0f, "selected squad failed to reach ordered x");
    expect(std::fabs(moved[2] - 250.0f) < 8.0f, "selected squad failed to reach ordered y");

    engine.reset();
    engine.setOrderMode(1);
    engine.tap(400.0f, 250.0f);
    for (int i = 0; i < 30; ++i) {
        engine.step(1.0f / 60.0f);
    }
    const std::vector<float> fastMoving = engine.unitSnapshot();
    expect(fastMoving[8] == 1.0f, "FAST order mode was not assigned");
    expect(fastMoving[1] > initial[1], "FAST order did not start movement");

    engine.stopSelected();
    const float stoppedX = engine.unitSnapshot()[1];
    for (int i = 0; i < 120; ++i) {
        engine.step(1.0f / 60.0f);
    }
    expect(std::fabs(engine.unitSnapshot()[1] - stoppedX) < 0.01f, "STOP did not halt selected squad");

    engine.reset();
    const float ammoBefore = unitAmmo(engine.unitSnapshot(), 0);
    engine.setOrderMode(3);
    engine.tap(400.0f, 250.0f);
    for (int i = 0; i < 180; ++i) {
        engine.step(1.0f / 60.0f);
    }
    const std::vector<float> firing = engine.unitSnapshot();
    expect(firing[8] == 3.0f, "FIRE order mode was not assigned");
    expect(unitAmmo(firing, 0) < ammoBefore, "FIRE order did not consume ammunition");

    engine.reset();
    const std::vector<float> reset = engine.unitSnapshot();
    expect(std::fabs(reset[1] - initial[1]) < 0.01f, "reset did not restore x");
    expect(std::fabs(reset[2] - initial[2]) < 0.01f, "reset did not restore y");
    expect(unitAmmo(reset, 0) == ammoBefore, "reset did not restore ammunition");

    std::cout << "engine_smoke: PASS\n";
    return 0;
}
