#pragma once

#include "FrameAnimation.h"
#include "SpriteSheet.h"
#include "../Physics/Collider.h"

#include <array>
#include <vector>

namespace downwell {

class Render;

enum class SpriteEffectType { BulletHit, Stomp, EnemyDisappear, Dust, Count };

// 衝突した位置を保存し、描画のために物理座標を動かさない。
struct SpriteEffectEvent {
    SpriteEffectType type;
    float x;
    float y;
    float angle = 0.0f;
};

// 短いエフェクト画像を共有し、各演出は独立して一度だけ再生する。
class SpriteEffects final {
public:
    bool SetSprite(SpriteEffectType type, std::shared_ptr<const SpriteSheet> sheet,
        const AnimationClip& clip, float scale, bool bottomAligned = false);
    bool Add(const SpriteEffectEvent& event);
    void Update(float deltaTime);
    void Draw(Render& render) const;
    // ステージを変える時は演出だけを消し、共有画像は引き継ぐ。
    void Clear() { m_effects.clear(); }

private:
    struct Sprite {
        std::shared_ptr<const SpriteSheet> sheet;
        AnimationClip clip;
        float scale = 1.0f;
        bool bottomAligned = false;
    };
    struct Effect {
        std::shared_ptr<const SpriteSheet> sheet;
        FrameAnimation animation;
        Rect bounds;
        float angle = 0.0f;
    };

    std::array<Sprite, static_cast<std::size_t>(SpriteEffectType::Count)> m_sprites{};
    std::vector<Effect> m_effects;
};

}
