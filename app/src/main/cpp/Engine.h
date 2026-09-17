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

struct CoverZone {
    Rect rect{};
    float cover{};
    float moveFactor{1.0f};
};

enum class OrderMode : int {
    Move = 0,
    Fast = 1,
    Sneak = 2,
    Fire = 3
};

enum class WeaponType : int {
    Rifle = 0,
    Smg = 1,
    Lmg = 2
};

struct Soldier {
    int id{};
    bool alive{true};
    float health{1.0f};
    WeaponType weapon{WeaponType::Rifle};
    int ammo{60};
    float reload{0.0f};
    float fatigue{0.0f};
};

struct Unit {
    int id{};
    int side{};
    Vec2 pos{};
    bool selected{};
    float morale{1.0f};
    float suppression{0.0f};
    float speed{115.0f};
    OrderMode orderMode{OrderMode::Move};
    std::vector<Vec2> path;
    std::size_t pathIndex{0};
    std::vector<Soldier> soldiers;
    bool hasFireTarget{false};
    Vec2 fireTarget{};
};

class Engine {
public:
    Engine();

    void reset();
    void step(float dt);
    void tap(float x, float y);
    void setOrderMode(int mode);
    void stopSelected();

    [[nodiscard]] std::vector<float> unitSnapshot() const;
    [[nodiscard]] std::vector<float> soldierSnapshot() const;
    [[nodiscard]] std::vector<float> obstacleSnapshot() const;
    [[nodiscard]] std::vector<float> coverSnapshot() const;
    [[nodiscard]] float worldWidth() const { return worldWidth_; }
    [[nodiscard]] float worldHeight() const { return worldHeight_; }

private:
    float worldWidth_{2400.0f};
    float worldHeight_{1400.0f};
    float cellSize_{40.0f};

    std::vector<Rect> obstacles_;
    std::vector<CoverZone> coverZones_;
    std::vector<Unit> units_;
    std::uint32_t rng_{0xC10C0A7u};
    float accumulator_{0.0f};
    OrderMode pendingOrderMode_{OrderMode::Move};

    void tick(float dt);
    void updateMovement(Unit& unit, float dt);
    void updateCombat(Unit& unit, float dt);

    [[nodiscard]] bool pointBlocked(Vec2 p, float padding = 0.0f) const;
    [[nodiscard]] bool cellBlocked(int cx, int cy) const;
    [[nodiscard]] bool hasLineOfSight(Vec2 a, Vec2 b) const;
    [[nodiscard]] std::vector<Vec2> findPath(Vec2 start, Vec2 goal) const;
    [[nodiscard]] float coverAt(Vec2 p) const;
    [[nodiscard]] float moveFactorAt(Vec2 p) const;
    [[nodiscard]] int livingCount(const Unit& unit) const;
    [[nodiscard]] int ammoCount(const Unit& unit) const;
    [[nodiscard]] float unitHealth(const Unit& unit) const;
    [[nodiscard]] int chooseTargetUnit(const Unit& shooter) const;
    [[nodiscard]] int chooseLivingSoldier(const Unit& unit);
    [[nodiscard]] float weaponRange(WeaponType type) const;
    [[nodiscard]] float weaponAccuracy(WeaponType type) const;
    [[nodiscard]] float weaponReload(WeaponType type) const;
    [[nodiscard]] float weaponDamage(WeaponType type);
    [[nodiscard]] float random01();
};

} // namespace cc
