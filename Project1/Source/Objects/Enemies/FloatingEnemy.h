#pragma once

#include "Enemy.h"

namespace downwell {

// 強い慣性を残しながら、主人公へまっすぐ進む浮遊敵。
class FloatingEnemy final : public Enemy {
public:
    FloatingEnemy(float x, float y, int health = DefaultHealth);
    void HitByBullet(int damage = DefaultDamage) override;
    bool CanStomp() const override { return true; }

    static constexpr int DefaultHealth = 3;
};

}
