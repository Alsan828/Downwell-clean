#pragma once

#include "../GameObject.h"
#include "../Enemies/Enemy.h"
#include "../../Stage/Blocks/Block.h"

namespace downwell {

// 下向きの弾。移動途中の接触も調べ、最初の一体・一マスだけに当たる。
class Bullet final : public GameObject {
public:
    Bullet(float x, float y, float speed, int damage);

    void Update(float deltaTime, BlockList& blocks, EnemyList* enemies = nullptr);
    void Draw(Render& render) const override;

    static constexpr int BulletWidth = 10;
    static constexpr int BulletHeight = 18;
    bool Impacted() const { return m_impacted; }
    float ImpactY() const { return GetCollision().bottom; }

private:
    float m_speed;
    int m_damage;
    float m_time = 0.0f;
    bool m_impacted = false;
    Rect m_previousDrawBounds{};

    static constexpr float Lifetime = .5f;
    static constexpr float SpriteScale = 2.0f;
    static constexpr float DownwardSpriteAngle = 1.570796327f;
};

}
