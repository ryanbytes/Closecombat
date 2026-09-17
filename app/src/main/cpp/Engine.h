#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cc {

struct Vec2 {
    float x{};
    float y{};
};

struct Rect {
    float x{};
    float y{};
    float w{};
    float h{};
};

struct Unit {
    int id{};
    int side{};
    Vec2 pos{};
    bool selected{};
    float morale{1.0f};
    float suppression{0.0f};
    float health{1.0f};
    float speed{115.0f};
    float reload{0.0f};
    std::vector<Vec2> path;
    std::size_t pathIndex{0};
};

class Engine {
public:
    Engine();

    void reset();
    void step(float dt);
    void tap(float x, float y);

    [[nodiscard]] std::vector<float> unitSnapshot() const;
    [[nodiscard]] std::vector<float> obstacleSnapshot() const;
    [[nodiscard]] float worldWidth() const { return worldWidth_; }
    [[nodiscard]] float worldHeight() const { return worldHeight_; }

private:
    float worldWidth_{2400.0f};
    float worldHeight_{1400.0f};
    float cellSize_{40.0f};

    std::vector<Rect> obstacles_;
    std::vector<Unit> units_;
    std::uint32_t rng_{0xC10C0A7u};
    float accumulator_{0.0f};

    void tick(float dt);
    [[nodiscard]] bool pointBlocked(Vec2 p, float padding = 0.0f) const;
    [[nodiscard]] bool cellBlocked(int cx, int cy) const;
    [[nodiscard]] bool hasLineOfSight(Vec2 a, Vec2 b) const;
    [[nodiscard]] std::vector<Vec2> findPath(Vec2 start, Vec2 goal) const;
    [[nodiscard]] float random01();
};

} // namespace cc
