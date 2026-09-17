#include "Engine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>

namespace cc {
namespace {

float distanceSquared(Vec2 a, Vec2 b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

float clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

bool pointInRect(Vec2 p, const Rect& rect) {
    return p.x >= rect.x && p.x <= rect.x + rect.w
            && p.y >= rect.y && p.y <= rect.y + rect.h;
}

} // namespace

Engine::Engine() {
    reset();
}

void Engine::reset() {
    rng_ = 0xC10C0A7u;
    accumulator_ = 0.0f;
    pendingOrderMode_ = OrderMode::Move;

    obstacles_ = {
        {520.0f, 190.0f, 310.0f, 220.0f},
        {980.0f, 80.0f, 220.0f, 420.0f},
        {1390.0f, 300.0f, 380.0f, 190.0f},
        {370.0f, 760.0f, 430.0f, 180.0f},
        {1050.0f, 710.0f, 270.0f, 310.0f},
        {1600.0f, 850.0f, 420.0f, 180.0f}
    };

    coverZones_ = {
        {{300.0f, 160.0f, 160.0f, 300.0f}, 0.28f, 0.88f},
        {{840.0f, 510.0f, 330.0f, 120.0f}, 0.42f, 0.78f},
        {{1340.0f, 570.0f, 260.0f, 180.0f}, 0.34f, 0.84f},
        {{1840.0f, 170.0f, 260.0f, 260.0f}, 0.30f, 0.86f},
        {{1330.0f, 1080.0f, 440.0f, 150.0f}, 0.50f, 0.72f}
    };

    units_.clear();
    int unitId = 1;
    int soldierId = 1;

    auto makeSquad = [&](int side, Vec2 pos) {
        Unit unit;
        unit.id = unitId++;
        unit.side = side;
        unit.pos = pos;

        for (int i = 0; i < 5; ++i) {
            Soldier soldier;
            soldier.id = soldierId++;
            if (i == 0) {
                soldier.weapon = WeaponType::Lmg;
                soldier.ammo = 120;
            } else if (i == 1) {
                soldier.weapon = WeaponType::Smg;
                soldier.ammo = 90;
            } else {
                soldier.weapon = WeaponType::Rifle;
                soldier.ammo = 60;
            }
            unit.soldiers.push_back(soldier);
        }
        return unit;
    };

    const std::array<Vec2, 6> friendly = {{
        {180.0f, 250.0f},
        {220.0f, 330.0f},
        {170.0f, 420.0f},
        {250.0f, 520.0f},
        {190.0f, 610.0f},
        {290.0f, 690.0f}
    }};
    for (Vec2 pos : friendly) {
        units_.push_back(makeSquad(0, pos));
    }

    const std::array<Vec2, 6> enemy = {{
        {2100.0f, 280.0f},
        {2170.0f, 390.0f},
        {2050.0f, 520.0f},
        {2210.0f, 720.0f},
        {2020.0f, 890.0f},
        {2160.0f, 1040.0f}
    }};
    for (Vec2 pos : enemy) {
        units_.push_back(makeSquad(1, pos));
    }

    if (!units_.empty()) {
        units_.front().selected = true;
    }
}

void Engine::step(float dt) {
    constexpr float kTickSeconds = 1.0f / 30.0f;
    constexpr int kMaxTicksPerCall = 8;

    accumulator_ += std::max(0.0f, std::min(dt, 0.25f));

    int ticks = 0;
    while (accumulator_ + 0.000001f >= kTickSeconds && ticks < kMaxTicksPerCall) {
        tick(kTickSeconds);
        accumulator_ -= kTickSeconds;
        ++ticks;
    }

    if (ticks == kMaxTicksPerCall && accumulator_ >= kTickSeconds) {
        accumulator_ = std::fmod(accumulator_, kTickSeconds);
    }
}

void Engine::tick(float dt) {
    for (Unit& unit : units_) {
        if (livingCount(unit) <= 0) {
            unit.selected = false;
            unit.path.clear();
            unit.pathIndex = 0;
            continue;
        }

        unit.suppression = clamp01(unit.suppression - dt * 0.045f);
        if (unit.suppression < 0.25f) {
            unit.morale = clamp01(unit.morale + dt * 0.018f);
        }

        for (Soldier& soldier : unit.soldiers) {
            if (!soldier.alive) {
                continue;
            }
            soldier.reload = std::max(0.0f, soldier.reload - dt);
            const float fatigueRecovery = unit.pathIndex < unit.path.size() ? 0.015f : 0.050f;
            soldier.fatigue = clamp01(soldier.fatigue - dt * fatigueRecovery);
        }

        updateMovement(unit, dt);
    }

    for (Unit& unit : units_) {
        if (livingCount(unit) > 0) {
            updateCombat(unit, dt);
        }
    }
}

void Engine::updateMovement(Unit& unit, float dt) {
    if (unit.orderMode == OrderMode::Fire || unit.pathIndex >= unit.path.size() || unit.suppression >= 0.92f) {
        return;
    }

    const Vec2 target = unit.path[unit.pathIndex];
    const float dx = target.x - unit.pos.x;
    const float dy = target.y - unit.pos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 1.0f) {
        ++unit.pathIndex;
        return;
    }

    float orderSpeedFactor = 1.0f;
    float fatigueRate = 0.010f;
    switch (unit.orderMode) {
        case OrderMode::Fast:
            orderSpeedFactor = 1.35f;
            fatigueRate = 0.085f;
            break;
        case OrderMode::Sneak:
            orderSpeedFactor = 0.55f;
            fatigueRate = 0.005f;
            break;
        case OrderMode::Move:
        default:
            break;
    }

    float averageFatigue = 0.0f;
    int alive = 0;
    for (Soldier& soldier : unit.soldiers) {
        if (!soldier.alive) {
            continue;
        }
        soldier.fatigue = clamp01(soldier.fatigue + dt * fatigueRate);
        averageFatigue += soldier.fatigue;
        ++alive;
    }
    if (alive > 0) {
        averageFatigue /= static_cast<float>(alive);
    }

    const float stateFactor = std::max(
            0.18f,
            unit.morale * (1.0f - 0.70f * unit.suppression) * (1.0f - 0.45f * averageFatigue));
    const float travel = unit.speed * orderSpeedFactor * moveFactorAt(unit.pos) * stateFactor * dt;

    if (travel >= dist) {
        unit.pos = target;
        ++unit.pathIndex;
    } else {
        unit.pos.x += dx / dist * travel;
        unit.pos.y += dy / dist * travel;
    }
}


void Engine::updateCombat(Unit& shooter, float dt) {
    (void)dt;

    const int targetIndex = chooseTargetUnit(shooter);
    Unit* target = targetIndex >= 0 ? &units_[static_cast<std::size_t>(targetIndex)] : nullptr;

    Vec2 aimPoint{};
    bool canAreaFire = false;
    if (shooter.orderMode == OrderMode::Fire && shooter.hasFireTarget) {
        aimPoint = shooter.fireTarget;
        canAreaFire = hasLineOfSight(shooter.pos, aimPoint);
    } else if (target != nullptr) {
        aimPoint = target->pos;
    } else {
        return;
    }

    const float distance = std::sqrt(distanceSquared(shooter.pos, aimPoint));
    const float targetCover = target != nullptr ? coverAt(target->pos) : 0.0f;

    for (Soldier& soldier : shooter.soldiers) {
        if (!soldier.alive || soldier.ammo <= 0 || soldier.reload > 0.0f) {
            continue;
        }

        const float maxRange = weaponRange(soldier.weapon);
        if (distance > maxRange) {
            continue;
        }
        if (target != nullptr && !hasLineOfSight(shooter.pos, target->pos)) {
            continue;
        }
        if (target == nullptr && !canAreaFire) {
            continue;
        }

        --soldier.ammo;
        soldier.reload = weaponReload(soldier.weapon);

        const float rangeFactor = std::max(0.10f, 1.0f - distance / (maxRange * 1.20f));
        const float shooterState = (1.0f - shooter.suppression * 0.75f)
                * (0.55f + shooter.morale * 0.45f)
                * (1.0f - soldier.fatigue * 0.35f);

        if (target == nullptr) {
            continue;
        }

        const float hitChance = weaponAccuracy(soldier.weapon)
                * rangeFactor
                * shooterState
                * (1.0f - targetCover * 0.70f);

        const float suppressionGain = (0.020f + weaponAccuracy(soldier.weapon) * 0.055f)
                * (1.0f - targetCover * 0.45f);
        target->suppression = clamp01(target->suppression + suppressionGain);

        if (random01() < hitChance) {
            const int soldierIndex = chooseLivingSoldier(*target);
            if (soldierIndex >= 0) {
                Soldier& victim = target->soldiers[static_cast<std::size_t>(soldierIndex)];
                victim.health = clamp01(victim.health - weaponDamage(soldier.weapon));
                target->morale = clamp01(target->morale - 0.035f);

                if (victim.health <= 0.001f) {
                    victim.health = 0.0f;
                    victim.alive = false;
                    victim.ammo = 0;
                    target->morale = clamp01(target->morale - 0.10f);
                    target->suppression = clamp01(target->suppression + 0.15f);

                    if (livingCount(*target) <= 0) {
                        target->selected = false;
                        target->path.clear();
                        target->pathIndex = 0;
                    }
                }
            }
        }
    }
}

void Engine::tap(float x, float y) {
    const Vec2 point{x, y};

    int selectedIndex = -1;
    float best = 45.0f * 45.0f;

    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& unit = units_[i];
        if (unit.side != 0 || livingCount(unit) <= 0) {
            continue;
        }

        const float d2 = distanceSquared(unit.pos, point);
        if (d2 < best) {
            best = d2;
            selectedIndex = static_cast<int>(i);
        }
    }

    if (selectedIndex >= 0) {
        for (Unit& unit : units_) {
            if (unit.side == 0) {
                unit.selected = false;
            }
        }
        units_[static_cast<std::size_t>(selectedIndex)].selected = true;
        return;
    }

    for (Unit& unit : units_) {
        if (!unit.selected || unit.side != 0 || livingCount(unit) <= 0) {
            continue;
        }

        if (pendingOrderMode_ == OrderMode::Fire) {
            unit.orderMode = OrderMode::Fire;
            unit.path.clear();
            unit.pathIndex = 0;
            unit.hasFireTarget = true;
            unit.fireTarget = point;
            continue;
        }

        if (pointBlocked(point, 8.0f)) {
            continue;
        }

        const Vec2 destination{
            std::clamp(x, 20.0f, worldWidth_ - 20.0f),
            std::clamp(y, 20.0f, worldHeight_ - 20.0f)
        };

        unit.orderMode = pendingOrderMode_;
        unit.hasFireTarget = false;
        unit.path = findPath(unit.pos, destination);
        unit.pathIndex = 0;
    }
}

void Engine::setOrderMode(int mode) {
    switch (mode) {
        case 1:
            pendingOrderMode_ = OrderMode::Fast;
            break;
        case 2:
            pendingOrderMode_ = OrderMode::Sneak;
            break;
        case 3:
            pendingOrderMode_ = OrderMode::Fire;
            break;
        case 0:
        default:
            pendingOrderMode_ = OrderMode::Move;
            break;
    }
}

void Engine::stopSelected() {
    for (Unit& unit : units_) {
        if (unit.selected && unit.side == 0 && livingCount(unit) > 0) {
            unit.path.clear();
            unit.pathIndex = 0;
            unit.hasFireTarget = false;
            unit.orderMode = OrderMode::Move;
        }
    }
}

std::vector<float> Engine::unitSnapshot() const {
    std::vector<float> out;
    out.reserve(units_.size() * 12);

    for (const Unit& unit : units_) {
        out.push_back(static_cast<float>(unit.id));
        out.push_back(unit.pos.x);
        out.push_back(unit.pos.y);
        out.push_back(static_cast<float>(unit.side));
        out.push_back(unit.selected ? 1.0f : 0.0f);
        out.push_back(unit.morale);
        out.push_back(unit.suppression);
        out.push_back(unitHealth(unit));
        out.push_back(static_cast<float>(unit.orderMode));
        out.push_back(static_cast<float>(livingCount(unit)));
        out.push_back(static_cast<float>(unit.soldiers.size()));
        out.push_back(static_cast<float>(ammoCount(unit)));
    }
    return out;
}

std::vector<float> Engine::soldierSnapshot() const {
    std::vector<float> out;
    for (const Unit& unit : units_) {
        for (const Soldier& soldier : unit.soldiers) {
            out.push_back(static_cast<float>(unit.id));
            out.push_back(static_cast<float>(soldier.id));
            out.push_back(soldier.alive ? 1.0f : 0.0f);
            out.push_back(soldier.health);
            out.push_back(static_cast<float>(soldier.weapon));
            out.push_back(static_cast<float>(soldier.ammo));
            out.push_back(soldier.fatigue);
        }
    }
    return out;
}

std::vector<float> Engine::obstacleSnapshot() const {
    std::vector<float> out;
    out.reserve(obstacles_.size() * 4);
    for (const Rect& rect : obstacles_) {
        out.push_back(rect.x);
        out.push_back(rect.y);
        out.push_back(rect.w);
        out.push_back(rect.h);
    }
    return out;
}

std::vector<float> Engine::coverSnapshot() const {
    std::vector<float> out;
    out.reserve(coverZones_.size() * 6);
    for (const CoverZone& zone : coverZones_) {
        out.push_back(zone.rect.x);
        out.push_back(zone.rect.y);
        out.push_back(zone.rect.w);
        out.push_back(zone.rect.h);
        out.push_back(zone.cover);
        out.push_back(zone.moveFactor);
    }
    return out;
}

bool Engine::pointBlocked(Vec2 p, float padding) const {
    if (p.x < padding || p.y < padding || p.x > worldWidth_ - padding || p.y > worldHeight_ - padding) {
        return true;
    }

    for (const Rect& rect : obstacles_) {
        if (p.x >= rect.x - padding
                && p.x <= rect.x + rect.w + padding
                && p.y >= rect.y - padding
                && p.y <= rect.y + rect.h + padding) {
            return true;
        }
    }
    return false;
}

bool Engine::cellBlocked(int cx, int cy) const {
    const int cols = static_cast<int>(std::ceil(worldWidth_ / cellSize_));
    const int rows = static_cast<int>(std::ceil(worldHeight_ / cellSize_));
    if (cx < 0 || cy < 0 || cx >= cols || cy >= rows) {
        return true;
    }

    const Vec2 center{
        (static_cast<float>(cx) + 0.5f) * cellSize_,
        (static_cast<float>(cy) + 0.5f) * cellSize_
    };
    return pointBlocked(center, 16.0f);
}

bool Engine::hasLineOfSight(Vec2 a, Vec2 b) const {
    constexpr int samples = 48;
    for (int i = 1; i < samples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(samples);
        const Vec2 p{
            a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t
        };
        if (pointBlocked(p, 2.0f)) {
            return false;
        }
    }
    return true;
}

std::vector<Vec2> Engine::findPath(Vec2 start, Vec2 goal) const {
    const int cols = static_cast<int>(std::ceil(worldWidth_ / cellSize_));
    const int rows = static_cast<int>(std::ceil(worldHeight_ / cellSize_));
    const int count = cols * rows;

    auto indexOf = [cols](int x, int y) { return y * cols + x; };
    auto cellX = [cols](int index) { return index % cols; };
    auto cellY = [cols](int index) { return index / cols; };

    int sx = std::clamp(static_cast<int>(start.x / cellSize_), 0, cols - 1);
    int sy = std::clamp(static_cast<int>(start.y / cellSize_), 0, rows - 1);
    int gx = std::clamp(static_cast<int>(goal.x / cellSize_), 0, cols - 1);
    int gy = std::clamp(static_cast<int>(goal.y / cellSize_), 0, rows - 1);

    if (cellBlocked(gx, gy)) {
        bool found = false;
        for (int radius = 1; radius <= 6 && !found; ++radius) {
            for (int y = gy - radius; y <= gy + radius && !found; ++y) {
                for (int x = gx - radius; x <= gx + radius; ++x) {
                    if (!cellBlocked(x, y)) {
                        gx = x;
                        gy = y;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (!found) {
            return {};
        }
    }

    const int startIndex = indexOf(sx, sy);
    const int goalIndex = indexOf(gx, gy);

    std::vector<float> gScore(static_cast<std::size_t>(count), std::numeric_limits<float>::infinity());
    std::vector<int> cameFrom(static_cast<std::size_t>(count), -1);
    std::vector<bool> closed(static_cast<std::size_t>(count), false);

    using QueueItem = std::pair<float, int>;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> open;

    auto heuristic = [gx, gy](int x, int y) {
        const float dx = static_cast<float>(std::abs(gx - x));
        const float dy = static_cast<float>(std::abs(gy - y));
        const float diagonal = std::min(dx, dy);
        const float straight = dx + dy - 2.0f * diagonal;
        return diagonal * 1.41421356f + straight;
    };

    gScore[static_cast<std::size_t>(startIndex)] = 0.0f;
    open.emplace(heuristic(sx, sy), startIndex);

    constexpr std::array<std::array<int, 2>, 8> directions = {{
        {{1, 0}}, {{-1, 0}}, {{0, 1}}, {{0, -1}},
        {{1, 1}}, {{1, -1}}, {{-1, 1}}, {{-1, -1}}
    }};

    while (!open.empty()) {
        const int current = open.top().second;
        open.pop();

        if (closed[static_cast<std::size_t>(current)]) {
            continue;
        }
        if (current == goalIndex) {
            break;
        }
        closed[static_cast<std::size_t>(current)] = true;

        const int cx = cellX(current);
        const int cy = cellY(current);

        for (const auto& direction : directions) {
            const int dx = direction[0];
            const int dy = direction[1];
            const int nx = cx + dx;
            const int ny = cy + dy;

            if (cellBlocked(nx, ny)) {
                continue;
            }
            if (dx != 0 && dy != 0 && (cellBlocked(cx + dx, cy) || cellBlocked(cx, cy + dy))) {
                continue;
            }

            const int next = indexOf(nx, ny);
            const float stepCost = (dx != 0 && dy != 0) ? 1.41421356f : 1.0f;
            const float tentative = gScore[static_cast<std::size_t>(current)] + stepCost;

            if (tentative < gScore[static_cast<std::size_t>(next)]) {
                cameFrom[static_cast<std::size_t>(next)] = current;
                gScore[static_cast<std::size_t>(next)] = tentative;
                open.emplace(tentative + heuristic(nx, ny), next);
            }
        }
    }

    if (goalIndex != startIndex && cameFrom[static_cast<std::size_t>(goalIndex)] < 0) {
        return {};
    }

    std::vector<Vec2> reversed;
    int cursor = goalIndex;
    while (cursor != startIndex) {
        const int x = cellX(cursor);
        const int y = cellY(cursor);
        reversed.push_back({
            (static_cast<float>(x) + 0.5f) * cellSize_,
            (static_cast<float>(y) + 0.5f) * cellSize_
        });
        cursor = cameFrom[static_cast<std::size_t>(cursor)];
        if (cursor < 0) {
            return {};
        }
    }

    std::reverse(reversed.begin(), reversed.end());
    if (!pointBlocked(goal, 8.0f)) {
        reversed.push_back(goal);
    }
    return reversed;
}

float Engine::coverAt(Vec2 p) const {
    float result = 0.0f;
    for (const CoverZone& zone : coverZones_) {
        if (pointInRect(p, zone.rect)) {
            result = std::max(result, zone.cover);
        }
    }
    return result;
}

float Engine::moveFactorAt(Vec2 p) const {
    float result = 1.0f;
    for (const CoverZone& zone : coverZones_) {
        if (pointInRect(p, zone.rect)) {
            result = std::min(result, zone.moveFactor);
        }
    }
    return result;
}

int Engine::livingCount(const Unit& unit) const {
    int count = 0;
    for (const Soldier& soldier : unit.soldiers) {
        if (soldier.alive) {
            ++count;
        }
    }
    return count;
}

int Engine::ammoCount(const Unit& unit) const {
    int total = 0;
    for (const Soldier& soldier : unit.soldiers) {
        if (soldier.alive) {
            total += soldier.ammo;
        }
    }
    return total;
}

float Engine::unitHealth(const Unit& unit) const {
    if (unit.soldiers.empty()) {
        return 0.0f;
    }
    float health = 0.0f;
    for (const Soldier& soldier : unit.soldiers) {
        health += soldier.alive ? soldier.health : 0.0f;
    }
    return health / static_cast<float>(unit.soldiers.size());
}

int Engine::chooseTargetUnit(const Unit& shooter) const {
    int bestTarget = -1;
    float bestDistanceSquared = 550.0f * 550.0f;

    if (shooter.orderMode == OrderMode::Fire && shooter.hasFireTarget) {
        bestDistanceSquared = 150.0f * 150.0f;
        for (std::size_t i = 0; i < units_.size(); ++i) {
            const Unit& candidate = units_[i];
            if (candidate.side == shooter.side || livingCount(candidate) <= 0) {
                continue;
            }
            const float d2 = distanceSquared(candidate.pos, shooter.fireTarget);
            if (d2 < bestDistanceSquared && hasLineOfSight(shooter.pos, candidate.pos)) {
                bestDistanceSquared = d2;
                bestTarget = static_cast<int>(i);
            }
        }
        return bestTarget;
    }

    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& candidate = units_[i];
        if (candidate.side == shooter.side || livingCount(candidate) <= 0) {
            continue;
        }

        const float d2 = distanceSquared(shooter.pos, candidate.pos);
        if (d2 < bestDistanceSquared && hasLineOfSight(shooter.pos, candidate.pos)) {
            bestDistanceSquared = d2;
            bestTarget = static_cast<int>(i);
        }
    }
    return bestTarget;
}

int Engine::chooseLivingSoldier(const Unit& unit) {
    std::vector<int> living;
    for (std::size_t i = 0; i < unit.soldiers.size(); ++i) {
        if (unit.soldiers[i].alive) {
            living.push_back(static_cast<int>(i));
        }
    }
    if (living.empty()) {
        return -1;
    }
    const std::size_t pick = static_cast<std::size_t>(random01() * static_cast<float>(living.size()));
    return living[std::min(pick, living.size() - 1)];
}

float Engine::weaponRange(WeaponType type) const {
    switch (type) {
        case WeaponType::Smg:
            return 330.0f;
        case WeaponType::Lmg:
            return 650.0f;
        case WeaponType::Rifle:
        default:
            return 560.0f;
    }
}

float Engine::weaponAccuracy(WeaponType type) const {
    switch (type) {
        case WeaponType::Smg:
            return 0.22f;
        case WeaponType::Lmg:
            return 0.24f;
        case WeaponType::Rifle:
        default:
            return 0.31f;
    }
}

float Engine::weaponReload(WeaponType type) const {
    switch (type) {
        case WeaponType::Smg:
            return 0.28f;
        case WeaponType::Lmg:
            return 0.22f;
        case WeaponType::Rifle:
        default:
            return 0.72f;
    }
}

float Engine::weaponDamage(WeaponType type) {
    switch (type) {
        case WeaponType::Smg:
            return 0.30f + random01() * 0.22f;
        case WeaponType::Lmg:
            return 0.34f + random01() * 0.25f;
        case WeaponType::Rifle:
        default:
            return 0.45f + random01() * 0.28f;
    }
}

float Engine::random01() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return static_cast<float>((rng_ >> 8u) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

} // namespace cc
