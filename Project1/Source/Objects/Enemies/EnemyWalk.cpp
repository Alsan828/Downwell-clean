#include "EnemyAI.h"
#include "Enemy.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace downwell {

namespace {

constexpr float FloorTolerance = 0.001f;

bool FindFloor(const Enemy& enemy, const BlockList& blocks, float& left, float& right) {
    const Rect& bounds = enemy.GetCollision();
    std::vector<std::pair<float, float>> ranges;
    for (const Block* block : blocks.Nearby(bounds.bottom - FloorTolerance, bounds.bottom + FloorTolerance)) {
        if (!block->Destroyed() && std::abs(block->Top() - bounds.bottom) <= FloorTolerance) {
            ranges.emplace_back(block->Left(), block->Right());
        }
    }
    std::sort(ranges.begin(), ranges.end());

    // 同じ高さの足場だけをつなぐ。隙間があれば崖として扱う。
    for (std::size_t index = 0; index < ranges.size();) {
        left = ranges[index].first;
        right = ranges[index].second;
        ++index;
        while (index < ranges.size() && ranges[index].first <= right + FloorTolerance) {
            right = std::max(right, ranges[index].second);
            ++index;
        }
        // 落下時と同じく、体の一部が足場に重なれば支えられている。
        if (bounds.right > left && bounds.left < right) return true;
    }
    return false;
}

}

double EnemyAI::NextWalkPhaseDuration() {
    static std::mt19937 random{ std::random_device{}() };
    return std::uniform_real_distribution<double>(WalkMinPhaseDuration, WalkMaxPhaseDuration)(random);
}

void EnemyAI::StepWalking(Enemy& enemy, double deltaTime, const BlockList& blocks) {
    float floorLeft = 0.0f, floorRight = 0.0f;
    const bool falling = !m_attached || !FindFloor(enemy, blocks, floorLeft, floorRight);
    if (falling) {
        // 重力は固定ステップに一度だけ適用し、着地した次のステップから歩く。
        m_attached = false;
        Fall(enemy, static_cast<float>(deltaTime), blocks);
        m_animationDeltaTime += deltaTime;
    }
    const auto nextPhase = [this] {
        // 各個体で、移動と休憩の長さを毎回決め直す。
        m_walking = !m_walking;
        m_walkTimeRemaining += NextWalkPhaseDuration();
    };
    double remaining = deltaTime;
    while (remaining > 0.0) {
        if (m_walkTimeRemaining <= WalkPhaseTolerance) nextPhase();

        // 境界の前後へ時間を分け、休憩分まで歩かせない。
        const double phaseTime = std::min(remaining, m_walkTimeRemaining);
        if (!falling && StepWalkingPhase(enemy, static_cast<float>(phaseTime), blocks, floorLeft, floorRight)) {
            m_animationDeltaTime += phaseTime;
        }
        m_walkTimeRemaining -= phaseTime;
        remaining -= phaseTime;
        if (m_walkTimeRemaining <= WalkPhaseTolerance) nextPhase();
    }
}

bool EnemyAI::StepWalkingPhase(Enemy& enemy, float deltaTime, const BlockList& blocks,
    float floorLeft, float floorRight) {
    if (!m_walking) return false;

    const Rect& bounds = enemy.GetCollision();
    const float halfWidth = enemy.Width() * 0.5f;
    // 先端が足場を離れる前に折り返す。壁には上らない。
    const float edge = m_crawlDirection > 0 ? floorRight - halfWidth : floorLeft + halfWidth;
    float distance = std::max(0.0f, (edge - enemy.X()) * m_crawlDirection);

    for (const Block* block : blocks.Nearby(bounds.top, bounds.bottom)) {
        if (block->Destroyed() || bounds.bottom <= block->Top() + FloorTolerance ||
            bounds.top >= block->Bottom() - FloorTolerance) continue;

        const float gap = m_crawlDirection > 0 ? block->Left() - bounds.right : bounds.left - block->Right();
        if (gap >= -FloorTolerance) distance = std::min(distance, std::max(0.0f, gap));
    }

    const float step = CrawlSpeed * deltaTime;
    const float travel = std::min(step, distance);
    enemy.SetPosition(enemy.X() + travel * m_crawlDirection, enemy.Y());
    if (distance <= step + FloorTolerance) m_crawlDirection = -m_crawlDirection;
    return true;
}

}
