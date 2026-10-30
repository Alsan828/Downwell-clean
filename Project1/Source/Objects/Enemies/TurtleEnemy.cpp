#include "TurtleEnemy.h"

namespace downwell {

TurtleEnemy::TurtleEnemy(float x, float y, int health, int direction)
    : Enemy(x, y, true, health, BodyWidth, BodyHeight) {
    m_ai.SetWalking(direction);
}

void TurtleEnemy::HitByBullet(int) {
    // 甲羅で弾を止める。体力は減らさず、踏み付けで倒す。
}

}
