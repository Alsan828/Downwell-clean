#include "Player.h"

#include <algorithm>

namespace downwell {

void Player::BounceFromStomp(float enemyTop) {
    m_events.push_back(Event::Stomp);
    m_spriteEvents.push_back({ SpriteEffectType::Stomp, m_x, enemyTop });
    SetPosition(m_x, enemyTop - Height() * 0.5f);
    m_verticalSpeed = -StompSpeed;
    m_wallJumpDrift = false;
    m_softenApex = true;
    m_grounded = false;
    m_landingHopReady = false;
    m_jumpCut = true;
    ClearJumpBrake();
}

bool Player::TakeHit(float sourceX) {
    if (m_dying || Hurt()) return false;

    // 短い無敵時間を設け、接触したまま毎刻み被弾することを防ぐ。
    m_health = std::max(MinHealth, m_health - HitDamage);
    m_events.push_back(Event::Hurt);
    m_hurtTime = HurtDuration;
    m_knockbackTime = KnockbackDuration;
    m_wallJumpDrift = false;
    m_knockbackDirection = m_x < sourceX ? -1 : 1;
    m_horizontalSpeed = m_knockbackDirection * KnockbackSpeed;
    m_verticalSpeed = -HurtJumpSpeed;
    m_softenApex = false;
    m_grounded = false;
    m_landingHopReady = false;
    m_jumpCut = true;
    ClearJumpBrake();
    return true;
}

}
