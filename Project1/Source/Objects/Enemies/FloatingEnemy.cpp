#include "FloatingEnemy.h"

namespace downwell {

FloatingEnemy::FloatingEnemy(float x, float y, int health)
    : Enemy(x, y, true, health) {
    m_ai.SetChasing();
}

void FloatingEnemy::HitByBullet(int damage) {
    if (!Active() || damage <= 0) return;

    Hit(damage);
    // 生き残ったときだけ、弾の進む下方向へ押し戻す。
    if (Active()) m_ai.ApplyBulletKnockback();
}

}
