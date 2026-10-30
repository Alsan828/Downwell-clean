#include "EnemyAI.h"
#include "Enemy.h"

#include <cmath>

namespace downwell {

int EnemyAI::AnimationDirection() const {
    if (m_state == State::Crawling || m_state == State::Walking) return m_crawlDirection;
    if (m_state == State::Chasing) return m_velocityX > 0.0f ? 1 : (m_velocityX < 0.0f ? -1 : 0);
    return 0;
}

float EnemyAI::AnimationAngle() const {
    return m_state == State::Crawling && m_attached ? m_surface * SurfaceQuarterTurn : 0.0f;
}

bool EnemyAI::AnimationResting() const {
    return m_state == State::Walking && m_attached && !m_walking;
}

void EnemyAI::SetChasing() {
    m_state = State::Chasing;
    m_physicsAccumulator = 0.0;
    m_animationDeltaTime = 0.0;
    m_velocityX = 0.0f;
    m_velocityY = 0.0f;
    m_bulletKnockbackSpeed = 0.0f;
    m_bouncing = false;
}

void EnemyAI::ApplyBulletKnockback() {
    if (m_state != State::Chasing) return;

    // 連続命中でも力を足し続けず、横方向の慣性は残す。
    m_bulletKnockbackSpeed = BulletKnockbackSpeed;
    m_velocityY = 0.0f;
    // 天井で跳ねたときの古い位置へ戻さない。
    m_bouncing = false;
}

void EnemyAI::SetCrawling(int direction) {
    m_state = State::Crawling;
    m_crawlDirection = direction < 0 ? -1 : 1;
    m_surface = 0;
    m_attached = true;
    m_fallSpeed = 0.0f;
    m_physicsAccumulator = 0.0;
    m_animationDeltaTime = 0.0;
}

void EnemyAI::Update(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target) {
    m_animationDeltaTime = 0.0;
    if (!enemy.Active() || !std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    // 巡回だけは、各物理ステップで実際に動く・落ちる時間を集める。
    if (m_state != State::Walking) m_animationDeltaTime = deltaTime;
    if (m_state == State::Idle) return;

    UpdateMovement(enemy, deltaTime, blocks, target);
}

void EnemyAI::SetWalking(int direction) {
    SetCrawling(direction);
    m_state = State::Walking;
    m_walking = true;
    m_walkTimeRemaining = NextWalkPhaseDuration();
}

}
