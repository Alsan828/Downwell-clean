#pragma once

#include "Enemy.h"

namespace downwell {

// 足場の上、側面、下面を回る敵。踏み付けでも倒せない。
class GroundEnemy final : public Enemy {
public:
    GroundEnemy(float x, float y, int health = DefaultHealth, int direction = 1);

    bool Filled() const override { return true; }
    bool CanStomp() const override { return false; }

    static constexpr int DefaultHealth = 1;
};

}
