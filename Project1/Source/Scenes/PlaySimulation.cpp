#include "PlaySimulation.h"
#include "../Objects/Player.h"
#include "../Objects/Bullets/BulletSystem.h"
#include "../Core/InputManager.h"
#include "../Physics/PhysicsTiming.h"
#include "../Physics/Sweep.h"

#include <cmath>

namespace downwell {

namespace {

constexpr float StompTimeTolerance = 0.0001f;
constexpr float StompPositionTolerance = 0.01f;
constexpr float NoContactTime = 2.0f;

}

void PlaySimulation::Reset() {
    m_accumulator = 0.0;
    m_autoFire = false;
}

void PlaySimulation::Update(Player& player, const InputManager& input, float deltaTime, BlockList& blocks,
    EnemyList* enemies, BulletSystem* bullets) {
    player.ClearFrameEvents();
    if (bullets) bullets->ClearImpactEvents();
    if (player.Dying() || !std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    if (!input.Held(Action::JumpShoot)) m_autoFire = false;
    const auto action = player.ReadInput(input, blocks);
    if (action == Player::InputAction::Jump) {
        m_autoFire = false;
    } else if (action == Player::InputAction::Shoot) {
        m_autoFire = true;
        if (bullets) TryShoot(player, *bullets);
    }
    player.UpdateAnimation(0.0f);
    m_accumulator += deltaTime;

    while (m_accumulator + physics::AccumulatorTolerance >= physics::Step) {
        const float step = static_cast<float>(physics::Step);
        player.BeginPhysicsStep(step);
        if (bullets && m_autoFire) TryShoot(player, *bullets);
        const Rect previous = player.GetCollision();

        // 敵も同じ物理刻みで動かし、接触方向を移動前後から調べる。
        if (enemies) {
            for (auto& enemy : *enemies) enemy->Update(step, blocks, &player);
        }
        player.UpdatePhysics(step, blocks);
        // 足が触れた時点で連射を止める。着地の小跳びでも再開しない。
        if (player.Landed()) m_autoFire = false;
        if (enemies && ResolveContacts(player, previous, *enemies)) Reload(player, bullets, false);
        // 敵と弾を同じ物理刻みで更新し、被弾した対象の削除はStageに任せる。
        if (bullets) bullets->Update(step, blocks, enemies);
        player.UpdateAnimation(step);
        m_accumulator -= physics::Step;
        if (player.Health() == Player::MinHealth) break;
    }

    // 着地直後の小跳びがあっても、足が触れた時点で補充する。
    if (player.Landed()) Reload(player, bullets, true);
    player.SetInterpolationAlpha(static_cast<float>(m_accumulator / physics::Step));
}

void PlaySimulation::TryShoot(Player& player, BulletSystem& bullets) {
    if (player.CanShoot() && bullets.Fire(player.X(), player.GetCollision().bottom)) {
        player.ApplyShot(bullets.ShotFallSpeed());
    }
}

void PlaySimulation::Reload(Player& player, BulletSystem* bullets, bool playSound) {
    // 満弾なら補充しない。敵との接触では補充音を鳴らさない。
    if (!bullets || bullets->Ammo() >= bullets->Capacity()) return;

    bullets->Reload();
    if (playSound) player.NotifyReload();
}

bool PlaySimulation::ResolveContacts(Player& player, const Rect& previous, EnemyList& enemies) {
    Enemy* contact = nullptr;
    float firstTime = NoContactTime;
    bool stomp = false;
    const Rect current = player.GetCollision();

    for (auto& enemy : enemies) {
        if (!enemy->Active()) continue;

        const Rect& before = enemy->PreviousCollision();
        const Rect& after = enemy->GetCollision();
        const float dy = current.top - previous.top - (after.top - before.top);
        const float entry = SweepContactTime(previous, before, after,
            current.left - previous.left, current.top - previous.top);
        if (entry < 0.0f) continue;

        const float topTime = dy > 0.0f ? (before.top - previous.bottom) / dy : -1.0f;
        const bool fromAbove = dy > 0.0f && entry <= topTime + StompTimeTolerance &&
            previous.bottom <= before.top + StompPositionTolerance &&
            current.bottom >= after.top && player.VerticalSpeed() >= 0.0f && enemy->CanStomp();
        if (!fromAbove && player.Hurt()) continue;
        if (entry >= firstTime) continue;

        contact = enemy.get();
        firstTime = entry;
        stomp = fromAbove;
    }

    if (!contact) return false;
    if (!stomp) return player.TakeHit(contact->X());

    contact->Kill();
    player.BounceFromStomp(contact->GetCollision().top);
    return true;
}

}
