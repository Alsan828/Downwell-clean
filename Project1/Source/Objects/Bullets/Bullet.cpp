#include "Bullet.h"
#include "../../Graphics/Render.h"
#include "../../Graphics/SpriteSheet.h"
#include "../../Physics/Sweep.h"

#include <algorithm>
#include <cmath>

namespace downwell {

Bullet::Bullet(float x, float y, float speed, int damage)
    : GameObject(BulletWidth, BulletHeight), m_speed(speed), m_damage(damage) {
    SetPosition(x, y);
    m_previousDrawBounds = GetCollision();
}

void Bullet::Update(float deltaTime, BlockList& blocks, EnemyList* enemies) {
    if (!Active() || !std::isfinite(deltaTime) || deltaTime <= 0.0f) return;

    const float activeTime = std::min(deltaTime, Lifetime - m_time);
    const float activeRatio = activeTime / deltaTime;
    const float moveY = m_speed * activeTime;
    const Rect body = GetCollision();
    m_previousDrawBounds = body;
    float nearest = 2.0f;
    Block* hitBlock = nullptr;
    Enemy* hitEnemy = nullptr;

    for (Block* block : blocks.Nearby(std::min(body.top, body.top + moveY),
        std::max(body.bottom, body.bottom + moveY))) {
        if (block->Destroyed()) continue;
        const float time = SweepContactTime(body, block->GetCollision(), block->GetCollision(), 0.0f, moveY);
        if (time >= 0.0f && time < nearest) {
            nearest = time;
            hitBlock = block;
        }
    }

    if (enemies) {
        for (const auto& enemy : *enemies) {
            if (!enemy->Active()) continue;
            // 寿命が途中で終わる時は、敵の移動も同じ時間だけ調べる。
            const Rect& before = enemy->PreviousCollision();
            const Rect after = enemy->InterpolatedBounds(before, activeRatio);
            const float time = SweepContactTime(body, before, after, 0.0f, moveY);
            if (time >= 0.0f && time < nearest) {
                nearest = time;
                hitBlock = nullptr;
                hitEnemy = enemy.get();
            }
        }
    }

    if (hitBlock || hitEnemy) {
        SetPosition(m_x, m_y + moveY * nearest);
        m_impacted = true;
        if (hitBlock) hitBlock->Hit();
        if (hitEnemy) hitEnemy->HitByBullet(m_damage);
        m_active = false;
    } else {
        SetPosition(m_x, m_y + moveY);
    }

    m_time += deltaTime;
    if (m_time >= Lifetime) m_active = false;
}

void Bullet::Draw(Render& render) const {
    if (!Active()) return;

    const Rect body = InterpolatedBounds(m_previousDrawBounds, render.InterpolationAlpha());
    if (Sprite() && Sprite()->FrameCount() > 0) {
        // 右向きの原画像を下へ回し、弾の先端を判定の下端に合わせる。
        const float width = Sprite()->FrameWidth() * SpriteScale;
        const float height = Sprite()->FrameHeight() * SpriteScale;
        const float x = (body.left + body.right) * 0.5f;
        const float y = body.bottom - width * 0.5f;
        const Rect image{ x - width * 0.5f, y - height * 0.5f,
            x + width * 0.5f, y + height * 0.5f };
        if (DrawAnimation(render, image, DownwardSpriteAngle)) return;
    }
    render.DrawRectangle(body, 0xffffff);
}

}
