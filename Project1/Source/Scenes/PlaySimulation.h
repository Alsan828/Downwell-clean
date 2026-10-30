#pragma once

#include "../Stage/Blocks/Block.h"
#include "../Objects/Enemies/Enemy.h"

namespace downwell {

class Player;
class InputManager;
class BulletSystem;

// 主人公・敵・弾を同じ物理刻みと決まった順番で進める。
class PlaySimulation final {
public:
    // 新しいステージでは、余り時間と連射状態も戻す。
    void Reset();
    void Update(Player& player, const InputManager& input, float deltaTime,
        BlockList& blocks, EnemyList* enemies = nullptr, BulletSystem* bullets = nullptr);

private:
    void TryShoot(Player& player, BulletSystem& bullets);
    void Reload(Player& player, BulletSystem* bullets, bool playSound);
    bool ResolveContacts(Player& player, const Rect& previous, EnemyList& enemies);

    // 物理刻みの残り時間と、空中で押し直した連射だけを管理する。
    double m_accumulator = 0.0;
    bool m_autoFire = false;
};

}
