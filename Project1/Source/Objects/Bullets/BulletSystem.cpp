#include "BulletSystem.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace downwell {

BulletSystem::BulletSystem(WeaponSettings weapon)
    : m_weapon(weapon), m_ammo(0) {
    m_weapon.magazineSize = std::max(1, m_weapon.magazineSize);
    m_weapon.bulletSpeed = std::max(1.0f, m_weapon.bulletSpeed);
    m_weapon.damage = std::max(1, m_weapon.damage);
    m_weapon.fireInterval = std::max(0.0f, m_weapon.fireInterval);
    Reload();
}

bool BulletSystem::Fire(float x, float feetY) {
    if (m_ammo <= 0 || m_fireCooldown > 0.0) return false;

    m_bullets.emplace_back(x, feetY + Bullet::BulletHeight * 0.5f, m_weapon.bulletSpeed, m_weapon.damage);
    m_bullets.back().SetSpriteSheet(m_bulletSprite);
    --m_ammo;
    m_fireCooldown = m_weapon.fireInterval;
    return true;
}

void BulletSystem::Update(float deltaTime, BlockList& blocks, EnemyList* enemies) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    m_fireCooldown = std::max(0.0, m_fireCooldown - deltaTime);
    for (auto& bullet : m_bullets) {
        bullet.Update(deltaTime, blocks, enemies);
        if (bullet.Impacted()) {
            m_impactEvents.push_back({ SpriteEffectType::BulletHit, bullet.X(), bullet.ImpactY() });
        }
    }
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(), [](const Bullet& bullet) {
        return !bullet.Active();
    }), m_bullets.end());
}

void BulletSystem::Draw(Render& render) const {
    for (const auto& bullet : m_bullets) bullet.Draw(render);
}

bool BulletSystem::SetBulletSprite(std::shared_ptr<const SpriteSheet> sheet) {
    if (!sheet || sheet->FrameCount() == 0) return false;

    m_bulletSprite = std::move(sheet);
    for (auto& bullet : m_bullets) bullet.SetSpriteSheet(m_bulletSprite);
    return true;
}

void BulletSystem::Reload() {
    m_ammo = m_weapon.magazineSize;
}

void BulletSystem::Reset() {
    m_bullets.clear();
    m_impactEvents.clear();
    m_fireCooldown = 0.0;
    Reload();
}

}
