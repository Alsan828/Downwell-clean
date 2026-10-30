#pragma once

#include "Bullet.h"
#include "../../Graphics/SpriteEffects.h"

#include <vector>

namespace downwell {

// 銃ごとに変更できる弾数、弾速、ダメージ、発射間隔と発射後の上下方向の速度。
struct WeaponSettings {
    static constexpr int DefaultMagazineSize = 10;

    int magazineSize = DefaultMagazineSize;
    float bulletSpeed = 600.0f;
    int damage = 1;
    // 負の値は上向き。発射時にこの速度へ戻す。
    float shotFallSpeed = -35.0f;
    float fireInterval = 0.15f;
};

class BulletSystem final {
public:
    explicit BulletSystem(WeaponSettings weapon = {});

    bool Fire(float x, float feetY);
    void Update(float deltaTime, BlockList& blocks, EnemyList* enemies = nullptr);
    void Draw(Render& render) const;
    void Reload();
    void Reset();
    bool SetBulletSprite(std::shared_ptr<const SpriteSheet> sheet);

    int Ammo() const { return m_ammo; }
    int Capacity() const { return m_weapon.magazineSize; }
    float ShotFallSpeed() const { return m_weapon.shotFallSpeed; }
    const std::vector<Bullet>& Bullets() const { return m_bullets; }
    const std::vector<SpriteEffectEvent>& ImpactEvents() const { return m_impactEvents; }
    void ClearImpactEvents() { m_impactEvents.clear(); }

private:
    WeaponSettings m_weapon;
    int m_ammo;
    double m_fireCooldown = 0.0;
    std::vector<Bullet> m_bullets;
    std::vector<SpriteEffectEvent> m_impactEvents;
    std::shared_ptr<const SpriteSheet> m_bulletSprite;
};

}
