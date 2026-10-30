#include "Enemy.h"
#include "../../Graphics/Render.h"
#include "../../Graphics/SpriteSheet.h"

#include <algorithm>
#include <cmath>

namespace downwell {

bool EnemySprite::Valid() const {
    return sheet && clip.firstFrame >= 0 && clip.frameCount > 0 &&
        clip.firstFrame < sheet->FrameCount() &&
        clip.frameCount <= sheet->FrameCount() - clip.firstFrame &&
        std::isfinite(clip.frameDuration) && clip.frameDuration > 0.0f &&
        std::isfinite(scale) && scale > 0.0f;
}

Enemy::Enemy(float x, float y, bool stompable, int health, int width, int height)
    : GameObject(width, height), m_stompable(stompable), m_health(std::max(health, 1)) {
    SetPosition(x, y);
    m_previousCollision = GetCollision();
}

void Enemy::Update(float deltaTime, const BlockList& blocks, const GameObject* target) {
    if (!Active()) return;

    m_previousCollision = GetCollision();
    m_ai.Update(*this, deltaTime, blocks, target);
    const int direction = m_ai.AnimationDirection();
    if (direction != 0) m_facingRight = direction > 0;
    if (m_spriteScale > 0.0f) {
        // 最後の状態だけで止めず、境界前の有効な再生時間も進める。
        Frames().SetPaused(false);
        UpdateFrames(m_ai.AnimationDeltaTime());
        Frames().SetPaused(m_ai.AnimationResting());
    } else {
        UpdateFrames(deltaTime);
    }
}

bool Enemy::SetAnimationSprite(const EnemySprite& sprite) {
    if (!sprite.Valid()) return false;

    SetSpriteSheet(sprite.sheet);
    Frames().SetClip(sprite.clip);
    m_spriteScale = sprite.scale;
    m_spriteFacesRight = sprite.facesRight;
    const int direction = m_ai.AnimationDirection();
    if (direction != 0) m_facingRight = direction > 0;
    Frames().SetPaused(m_ai.AnimationResting());
    return true;
}

Rect Enemy::AnimationBounds(const Rect& bounds) const {
    if (!Sprite() || m_spriteScale <= 0.0f) return bounds;

    const float width = Sprite()->FrameWidth() * m_spriteScale;
    const float height = Sprite()->FrameHeight() * m_spriteScale;
    const float centerX = (bounds.left + bounds.right) * 0.5f;
    const float centerY = (bounds.top + bounds.bottom) * 0.5f;
    // 地上の絵は足元を合わせ、壁を回るときも元の判定中心を軸にする。
    const float offsetY = Filled() ? bounds.bottom - height * 0.5f - centerY : 0.0f;
    const float angle = AnimationAngle();
    const float drawX = centerX - std::sin(angle) * offsetY;
    const float drawY = centerY + std::cos(angle) * offsetY;
    return { drawX - width * 0.5f, drawY - height * 0.5f,
        drawX + width * 0.5f, drawY + height * 0.5f };
}

float Enemy::AnimationAngle() const {
    return m_spriteScale > 0.0f ? m_ai.AnimationAngle() : 0.0f;
}

bool Enemy::AnimationFlipX() const {
    return m_spriteScale > 0.0f && m_facingRight != m_spriteFacesRight;
}

void Enemy::Draw(Render& render) const {
    if (!Active()) return;

    const Rect bounds = InterpolatedBounds(m_previousCollision, render.InterpolationAlpha());
    if (DrawAnimation(render, AnimationBounds(bounds), AnimationAngle(), AnimationFlipX())) return;
    render.DrawRectangle(bounds, Color, Filled());
}

void Enemy::Hit(int damage) {
    if (!Active() || damage <= 0) return;

    // 弾などの攻撃は体力を減らし、残っていれば行動を続ける。
    if (damage >= m_health) {
        Kill();
    } else {
        m_health -= damage;
    }
}

void Enemy::HitByBullet(int damage) {
    Hit(damage);
}

void Enemy::Kill() {
    // 踏み付けは残り体力に関係なく倒す。
    m_health = 0;
    m_active = false;
}

}
