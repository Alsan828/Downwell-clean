#pragma once

#include "Enemy.h"

namespace downwell {

// 平らな足場を進み、各自4～6秒ずつ移動と休憩を繰り返す敵。
class TurtleEnemy final : public Enemy {
public:
    TurtleEnemy(float x, float y, int health = DefaultHealth, int direction = 1);
    void HitByBullet(int damage = DefaultDamage) override;

    bool Filled() const override { return true; }
    bool CanStomp() const override { return true; }

    static constexpr int BodyWidth = 32;
    static constexpr int BodyHeight = 18;
};

}
