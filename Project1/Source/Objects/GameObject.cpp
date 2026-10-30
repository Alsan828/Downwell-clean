#include "GameObject.h"
#include "../Graphics/Render.h"
#include "../Graphics/SpriteSheet.h"

#include <algorithm>
#include <utility>

namespace downwell {

GameObject::GameObject(int width, int height)
    : m_width(width), m_height(height) {
    m_collider.UpdateBounds(m_x, m_y, m_width, m_height);
}

void GameObject::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
    // 位置と判定範囲がずれないよう、移動時にまとめて更新する。
    m_collider.UpdateBounds(m_x, m_y, m_width, m_height);
}

void GameObject::Draw(Render& render) const {
    if (!Active()) return;

    if (DrawAnimation(render, GetCollision())) return;
    render.DrawRectangle(GetCollision(), 0xffffff);
}

void GameObject::SetSpriteSheet(std::shared_ptr<const SpriteSheet> sheet) {
    m_spriteSheet = std::move(sheet);
    Frames().Restart();
}

bool GameObject::DrawAnimation(Render& render, const Rect& bounds, float angle,
    bool flipX, unsigned int color, int alpha) const {
    return DrawAnimationFrame(render, bounds, Frames().Frame(), angle, flipX, color, alpha);
}

bool GameObject::DrawAnimationFrame(Render& render, const Rect& bounds, int frame, float angle,
    bool flipX, unsigned int color, int alpha) const {
    return m_spriteSheet && render.DrawFrame(*m_spriteSheet, frame, bounds, angle, flipX, color, alpha);
}

Rect GameObject::InterpolatedBounds(const Rect& previous, float alpha) const {
    // 描画だけを前後の物理位置で補間し、当たり判定は変更しない。
    alpha = std::clamp(alpha, 0.0f, 1.0f);
    const Rect& current = GetCollision();
    return {
        previous.left + (current.left - previous.left) * alpha,
        previous.top + (current.top - previous.top) * alpha,
        previous.right + (current.right - previous.right) * alpha,
        previous.bottom + (current.bottom - previous.bottom) * alpha
    };
}

}
