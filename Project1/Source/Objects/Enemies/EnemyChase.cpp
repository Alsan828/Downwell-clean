#include "EnemyAI.h"
#include "Enemy.h"

#include <algorithm>
#include <cmath>

namespace downwell {

namespace {

constexpr float FlightTolerance = 0.001f;

struct FlightContact {
    bool horizontal = false;
    bool vertical = false;
    bool ceiling = false;
};

FlightContact MoveFlying(Enemy& enemy, float moveX, float moveY, const BlockList& blocks) {
    FlightContact contact;
    const Rect before = enemy.GetCollision();
    float nextX = enemy.X() + moveX;
    for (const Block* block : blocks.Nearby(before.top, before.bottom)) {
        if (block->Destroyed() || before.bottom <= block->Top() || before.top >= block->Bottom()) continue;
        if (moveX > 0.0f && before.right <= block->Left() + FlightTolerance &&
            nextX + enemy.Width() * 0.5f > block->Left()) {
            nextX = block->Left() - enemy.Width() * 0.5f;
            contact.horizontal = true;
        } else if (moveX < 0.0f && before.left >= block->Right() - FlightTolerance &&
            nextX - enemy.Width() * 0.5f < block->Right()) {
            nextX = block->Right() + enemy.Width() * 0.5f;
            contact.horizontal = true;
        }
    }
    enemy.SetPosition(nextX, enemy.Y());

    const Rect afterX = enemy.GetCollision();
    float nextY = enemy.Y() + moveY;
    for (const Block* block : blocks.Nearby(afterX.top + std::min(0.0f, moveY) - FlightTolerance,
        afterX.bottom + std::max(0.0f, moveY) + FlightTolerance)) {
        if (block->Destroyed() || afterX.right <= block->Left() || afterX.left >= block->Right()) continue;
        if (moveY > 0.0f && afterX.bottom <= block->Top() + FlightTolerance &&
            nextY + enemy.Height() * 0.5f > block->Top()) {
            nextY = block->Top() - enemy.Height() * 0.5f;
            contact.vertical = true;
        } else if (moveY < 0.0f && afterX.top >= block->Bottom() - FlightTolerance &&
            nextY - enemy.Height() * 0.5f < block->Bottom()) {
            nextY = block->Bottom() + enemy.Height() * 0.5f;
            contact.vertical = true;
            contact.ceiling = true;
        }
    }
    enemy.SetPosition(enemy.X(), nextY);
    return contact;
}

}

void EnemyAI::StepChasing(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target) {
    if (m_bouncing) {
        // 天井から速く離れ、一マス先で止まる。横方向の慣性は残す。
        m_bounceTime = std::min(m_bounceTime + deltaTime, CeilingBounceDuration);
        const float t = m_bounceTime / CeilingBounceDuration;
        const float nextY = m_bounceOriginY + Block::TileSize * t * (2.0f - t);
        const FlightContact contact = MoveFlying(enemy, m_velocityX * deltaTime, nextY - enemy.Y(), blocks);
        if (contact.horizontal) m_velocityX = 0.0f;
        if (contact.vertical || m_bounceTime >= CeilingBounceDuration) {
            m_bouncing = false;
        }
        return;
    }
    float knockbackMoveY = 0.0f;
    if (m_bulletKnockbackSpeed > 0.0f) {
        // 指数減衰を積分して、最初は速く、徐々に止まる押し戻しにする。
        const float decay = std::exp(-deltaTime / BulletKnockbackDecayTime);
        knockbackMoveY = m_bulletKnockbackSpeed * BulletKnockbackDecayTime * (1.0f - decay);
        m_bulletKnockbackSpeed *= decay;
        if (m_bulletKnockbackSpeed < BulletKnockbackStopSpeed) m_bulletKnockbackSpeed = 0.0f;
    }

    const bool hasTarget = target && target->Active();
    if (!hasTarget && knockbackMoveY == 0.0f) return;

    float moveX = m_velocityX * deltaTime;
    float moveY = knockbackMoveY;
    if (hasTarget) {
        const float x = target->X() - enemy.X();
        const float y = target->Y() - enemy.Y();
        const float distance = std::hypot(x, y);
        float desiredX = 0.0f, desiredY = 0.0f;
        if (distance > FlightTolerance) {
            const float speed = std::min(ChaseSpeed, distance / ChaseSmoothTime);
            desiredX = x / distance * speed;
            desiredY = y / distance * speed;
        }

        // 強い慣性で元の進行方向を残し、主人公が動いてもすぐには反転しない。
        const float decay = std::exp(-deltaTime / ChaseSmoothTime);
        moveX = desiredX * deltaTime + (m_velocityX - desiredX) * ChaseSmoothTime * (1.0f - decay);
        moveY += desiredY * deltaTime + (m_velocityY - desiredY) * ChaseSmoothTime * (1.0f - decay);
        m_velocityX = desiredX + (m_velocityX - desiredX) * decay;
        m_velocityY = desiredY + (m_velocityY - desiredY) * decay;
    }

    const FlightContact contact = MoveFlying(enemy, moveX, moveY, blocks);
    if (contact.horizontal) m_velocityX = 0.0f;
    if (contact.vertical) {
        m_velocityY = 0.0f;
        m_bulletKnockbackSpeed = 0.0f;
    }
    if (contact.ceiling) {
        m_bouncing = true;
        m_bounceTime = 0.0f;
        m_bounceOriginY = enemy.Y();
        m_velocityY = 0.0f;
    }
}

}
