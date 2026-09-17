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

} // namespace

Engine::Engine() {
    reset();
}

void Engine::reset() {
    rng_ = 0xC10C0A7u;
    accumulator_ = 0.0f;
    pendingMoveMode_ = MoveMode::Move;

    obstacles_ = {
        {520.0f, 190.0f, 310.0f, 220.0f},
        {980.0f, 80.0f, 220.0f, 420.0f},
        {1390.0f, 300.0f, 380.0f, 190.0f},
        {370.0f, 760.0f, 430.0f, 180.0f},
        {1050.0f, 710.0f, 270.0f, 310.0f},
        {1600.0f, 850.0f, 420.0f, 180.0f}
    };

    units_.clear();
    int id = 1;

    const std::array<Vec2, 6> friendly = {{
        {180.0f, 250.0f},
        {220.0f, 330.0f},
        {170.0f, 420.0f},
        {250.0f, 520.0f},
        {190.0f, 610.0f},
        {290.0f, 690.0f}
    }};
    for (Vec2 pos : friendly) {
        Unit unit;
        unit.id = id++;
        unit.side = 0;
        unit.pos = pos;
        unit.selected = false;
        units_.push_back(unit);
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
        Unit unit;
        unit.id = id++;
        unit.side = 1;
        unit.pos = pos;
        unit.selected = false;
        units_.push_back(unit);
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
        if (unit.health <= 0.0f) {
            continue;
        }

        unit.reload = std::max(0.0f, unit.reload - dt);
        unit.suppression = clamp01(unit.suppression - dt * 0.045f);
        if (unit.suppression < 0.25f) {
            unit.morale = clamp01(unit.morale + dt * 0.018f);
        }

        if (unit.pathIndex < unit.path.size() && unit.suppression < 0.92f) {
            const Vec2 target = unit.path[unit.pathIndex];
            const float dx = target.x - unit.pos.x;
            const float dy = target.y - unit.pos.y;
            const float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < 1.0f) {
                ++unit.pathIndex;
            } else {
                const float stateFactor = std::max(0.22f, unit.morale * (1.0f - 0.70f * unit.suppression));
                float orderSpeedFactor = 1.0f;
                switch (unit.moveMode) {
                    case MoveMode::Fast:
                        orderSpeedFactor = 1.35f;
                        break;
                    case MoveMode::Sneak:
                        orderSpeedFactor = 0.55f;
                        break;
                    case MoveMode::Move:
                    default:
                        orderSpeedFactor = 1.0f;
                        break;
                }
                const float travel = unit.speed * orderSpeedFactor * stateFactor * dt;
                if (travel >= dist) {
                    unit.pos = target;
                    ++unit.pathIndex;
                } else {
                    unit.pos.x += dx / dist * travel;
                    unit.pos.y += dy / dist * travel;
                }
            }
        }
    }

    for (std::size_t i = 0; i < units_.size(); ++i) {
        Unit& shooter = units_[i];
        if (shooter.health <= 0.0f || shooter.morale < 0.12f || shooter.suppression > 0.95f) {
            continue;
        }

        std::size_t bestTarget = units_.size();
        float bestDist2 = 550.0f * 550.0f;

        for (std::size_t j = 0; j < units_.size(); ++j) {
            if (i == j || units_[j].side == shooter.side || units_[j].health <= 0.0f) {
                continue;
            }

            const float d2 = distanceSquared(shooter.pos, units_[j].pos);
            if (d2 < bestDist2 && hasLineOfSight(shooter.pos, units_[j].pos)) {
                bestDist2 = d2;
                bestTarget = j;
            }
        }

        if (bestTarget == units_.size() || shooter.reload > 0.0f) {
            continue;
        }

        Unit& target = units_[bestTarget];
        const float distance = std::sqrt(bestDist2);
        const float rangeFactor = std::max(0.20f, 1.0f - distance / 700.0f);
        const float hitChance = 0.34f * rangeFactor
                * (1.0f - shooter.suppression * 0.70f)
                * (0.55f + shooter.morale * 0.45f);

        target.suppression = clamp01(target.suppression + 0.10f + random01() * 0.08f);
        target.morale = clamp01(target.morale - 0.018f - target.suppression * 0.02f);

        if (random01() < hitChance) {
            target.health = clamp01(target.health - (0.08f + random01() * 0.12f));
            target.morale = clamp01(target.morale - 0.07f);
            if (target.health <= 0.001f) {
                target.health = 0.0f;
                target.path.clear();
                target.pathIndex = 0;
                target.selected = false;
            }
        }

        shooter.reload = 0.65f + random01() * 0.55f;
    }
}

void Engine::tap(float x, float y) {
    const Vec2 point{x, y};

    int selectedIndex = -1;
    float best = 45.0f * 45.0f;

    for (std::size_t i = 0; i < units_.size(); ++i) {
        const Unit& unit = units_[i];
        if (unit.side != 0 || unit.health <= 0.0f) {
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

    if (pointBlocked(point, 8.0f)) {
        return;
    }

    for (Unit& unit : units_) {
        if (!unit.selected || unit.side != 0 || unit.health <= 0.0f) {
            continue;
        }

        const Vec2 destination{
            std::clamp(x, 20.0f, worldWidth_ - 20.0f),
            std::clamp(y, 20.0f, worldHeight_ - 20.0f)
        };

        if (!pointBlocked(destination, 8.0f)) {
            unit.moveMode = pendingMoveMode_;
            unit.path = findPath(unit.pos, destination);
            unit.pathIndex = 0;
        }
    }
}

void Engine::setMoveMode(int mode) {
    switch (mode) {
        case 1:
            pendingMoveMode_ = MoveMode::Fast;
            break;
        case 2:
            pendingMoveMode_ = MoveMode::Sneak;
            break;
        case 0:
        default:
            pendingMoveMode_ = MoveMode::Move;
            break;
    }
}

void Engine::stopSelected() {
    for (Unit& unit : units_) {
        if (unit.selected && unit.side == 0 && unit.health > 0.0f) {
            unit.path.clear();
            unit.pathIndex = 0;
        }
    }
}

std::vector<float> Engine::unitSnapshot() const {
    std::vector<float> out;
    out.reserve(units_.size() * 9);

    for (const Unit& unit : units_) {
        out.push_back(static_cast<float>(unit.id));
        out.push_back(unit.pos.x);
        out.push_back(unit.pos.y);
        out.push_back(static_cast<float>(unit.side));
        out.push_back(unit.selected ? 1.0f : 0.0f);
        out.push_back(unit.morale);
        out.push_back(unit.suppression);
        out.push_back(unit.health);
        out.push_back(static_cast<float>(unit.moveMode));
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

float Engine::random01() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return static_cast<float>((rng_ >> 8u) & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

} // namespace cc
