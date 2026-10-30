#include "EnemyAI.h"
#include "Enemy.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace downwell {

namespace {

constexpr float SurfaceTolerance = 0.001f;
constexpr int MaxCornerTurns = 8;
constexpr int SurfaceCount = 4;

struct Direction {
    float x;
    float y;
};

// 上、右、下、左の順。法線は足場の内側、接線は時計回りを向く。
constexpr Direction Normals[SurfaceCount] = { { 0, 1 }, { -1, 0 }, { 0, -1 }, { 1, 0 } };
constexpr Direction Tangents[SurfaceCount] = { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };

struct SurfaceEnd {
    bool supported = false;
    bool obstacle = false;
    float distance = 0.0f;
};

SurfaceEnd FindSurface(const Enemy& enemy, Direction tangent, Direction normal, const BlockList& blocks) {
    const Rect& bounds = enemy.GetCollision();
    const float along = enemy.X() * tangent.x + enemy.Y() * tangent.y;
    const float across = enemy.X() * normal.x + enemy.Y() * normal.y;
    std::vector<std::pair<float, float>> supports;
    float obstacleDistance = std::numeric_limits<float>::max();

    for (const Block* block : blocks.Nearby(bounds.top - Block::TileSize, bounds.bottom + Block::TileSize)) {
        if (block->Destroyed()) continue;

        // 敵の半サイズだけ広げた地形の外周を、中心点でたどる。
        const Rect area{ block->Left() - enemy.Width() * 0.5f, block->Top() - enemy.Height() * 0.5f,
            block->Right() + enemy.Width() * 0.5f, block->Bottom() + enemy.Height() * 0.5f };
        const float a = area.left * tangent.x + area.top * tangent.y;
        const float b = area.right * tangent.x + area.bottom * tangent.y;
        const float nearAlong = std::min(a, b), farAlong = std::max(a, b);
        const float c = area.left * normal.x + area.top * normal.y;
        const float d = area.right * normal.x + area.bottom * normal.y;
        const float nearAcross = std::min(c, d), farAcross = std::max(c, d);

        if (std::abs(across - nearAcross) <= SurfaceTolerance) {
            supports.emplace_back(nearAlong, farAlong);
        } else if (across > nearAcross + SurfaceTolerance && across < farAcross - SurfaceTolerance &&
            farAlong > along + SurfaceTolerance) {
            obstacleDistance = std::min(obstacleDistance, std::max(0.0f, nearAlong - along));
        }
    }

    // 接したブロックの辺をつなぎ、区切りごとに向きが変わらないようにする。
    std::sort(supports.begin(), supports.end());
    float end = along;
    bool supported = false;
    for (const auto& range : supports) {
        if (range.second < along - SurfaceTolerance) continue;
        if (range.first > end + SurfaceTolerance) break;
        end = std::max(end, range.second);
        supported = true;
    }

    const float distance = std::max(0.0f, end - along);
    const bool obstacle = obstacleDistance < distance - SurfaceTolerance;
    return { supported, obstacle, obstacle ? obstacleDistance : distance };
}

}

void EnemyAI::UpdateMovement(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target) {
    m_physicsAccumulator += deltaTime;
    while (m_physicsAccumulator + physics::AccumulatorTolerance >= PhysicsStep) {
        if (m_state == State::Walking) StepWalking(enemy, PhysicsStep, blocks);
        else if (m_state == State::Chasing) StepChasing(enemy, static_cast<float>(PhysicsStep), blocks, target);
        else StepCrawling(enemy, static_cast<float>(PhysicsStep), blocks);
        m_physicsAccumulator -= PhysicsStep;
    }
}

void EnemyAI::StepCrawling(Enemy& enemy, float deltaTime, const BlockList& blocks) {
    if (!m_attached) {
        Fall(enemy, deltaTime, blocks);
        return;
    }

    float remaining = CrawlSpeed * deltaTime;
    for (int turn = 0; turn < MaxCornerTurns && remaining > SurfaceTolerance; ++turn) {
        const Direction tangent{ Tangents[m_surface].x * m_crawlDirection,
            Tangents[m_surface].y * m_crawlDirection };
        const SurfaceEnd surface = FindSurface(enemy, tangent, Normals[m_surface], blocks);
        if (!surface.supported) {
            // 支えていたブロックが壊れたら、近くの地形へ飛び移らず落下する。
            m_attached = false;
            Fall(enemy, deltaTime, blocks);
            return;
        }

        const float travel = std::min(remaining, surface.distance);
        enemy.SetPosition(enemy.X() + tangent.x * travel, enemy.Y() + tangent.y * travel);
        remaining -= travel;

        if (travel + SurfaceTolerance < surface.distance) break;

        // 外側の角は足場を回り込み、内側の角は前方の壁へ登る。
        const int rotation = surface.obstacle ? -m_crawlDirection : m_crawlDirection;
        m_surface = (m_surface + rotation + SurfaceCount) % SurfaceCount;
    }
}

void EnemyAI::Fall(Enemy& enemy, float deltaTime, const BlockList& blocks) {
    m_fallSpeed = std::min(m_fallSpeed + FallAcceleration * deltaTime, MaxFallSpeed);
    const Rect previous = enemy.GetCollision();
    float nextY = enemy.Y() + m_fallSpeed * deltaTime;
    bool landed = false;

    for (const Block* block : blocks.Nearby(previous.top - SurfaceTolerance,
        nextY + enemy.Height() * 0.5f + SurfaceTolerance)) {
        if (block->Destroyed() || previous.right <= block->Left() || previous.left >= block->Right()) continue;
        if (previous.bottom > block->Top() + SurfaceTolerance || nextY + enemy.Height() * 0.5f < block->Top()) continue;

        nextY = block->Top() - enemy.Height() * 0.5f;
        landed = true;
    }
    enemy.SetPosition(enemy.X(), nextY);

    if (landed) {
        m_surface = 0;
        m_attached = true;
        m_fallSpeed = 0.0f;
    }
}

}
