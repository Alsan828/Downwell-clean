#include "SpriteEffects.h"
#include "Render.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace downwell {

bool SpriteEffects::SetSprite(SpriteEffectType type, std::shared_ptr<const SpriteSheet> sheet,
    const AnimationClip& clip, float scale, bool bottomAligned) {
    const auto index = static_cast<std::size_t>(type);
    if (index >= m_sprites.size() || !sheet || !std::isfinite(scale) || scale <= 0.0f ||
        clip.firstFrame < 0 || clip.frameCount < 1 ||
        clip.firstFrame > sheet->FrameCount() - clip.frameCount) return false;

    AnimationClip once = clip;
    once.loop = false;
    m_sprites[index] = { std::move(sheet), once, scale, bottomAligned };
    return true;
}

bool SpriteEffects::Add(const SpriteEffectEvent& event) {
    const auto index = static_cast<std::size_t>(event.type);
    if (index >= m_sprites.size() || !std::isfinite(event.x) || !std::isfinite(event.y) ||
        !std::isfinite(event.angle)) return false;

    const auto& sprite = m_sprites[index];
    if (!sprite.sheet || sprite.sheet->FrameCount() == 0) return false;

    const float width = sprite.sheet->FrameWidth() * sprite.scale;
    const float height = sprite.sheet->FrameHeight() * sprite.scale;
    // 足元の位置を軸に回し、壁の砂ぼこりも接触面に合わせる。
    const float offsetY = sprite.bottomAligned ? -height * 0.5f : 0.0f;
    const float centerX = event.x - std::sin(event.angle) * offsetY;
    const float centerY = event.y + std::cos(event.angle) * offsetY;
    Effect effect{ sprite.sheet, {},
        { centerX - width * 0.5f, centerY - height * 0.5f,
            centerX + width * 0.5f, centerY + height * 0.5f }, event.angle };
    effect.animation.SetClip(sprite.clip);
    m_effects.push_back(std::move(effect));
    return true;
}

void SpriteEffects::Update(float deltaTime) {
    for (auto& effect : m_effects) effect.animation.Update(deltaTime);
    m_effects.erase(std::remove_if(m_effects.begin(), m_effects.end(), [](const Effect& effect) {
        return effect.animation.Finished();
    }), m_effects.end());
}

void SpriteEffects::Draw(Render& render) const {
    for (const auto& effect : m_effects) {
        render.DrawFrame(*effect.sheet, effect.animation.Frame(), effect.bounds, effect.angle);
    }
}

}
