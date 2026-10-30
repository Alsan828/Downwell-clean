#include "GroundEnemy.h"

namespace downwell {

GroundEnemy::GroundEnemy(float x, float y, int health, int direction)
    : Enemy(x, y, false, health) {
    m_ai.SetCrawling(direction);
}

}
