#pragma once

#include "../Physics/Collider.h"
#include "../Graphics/FrameAnimation.h"

#include <memory>

namespace downwell {

class Render;
class SpriteSheet;

// ゲーム内のオブジェクトが共有する位置、大きさ、当たり判定。
class GameObject {
public:
    virtual ~GameObject() = default;

    void SetPosition(float x, float y);
    virtual void Draw(Render& render) const;

    // 画像は共有し、再生位置は物体ごとに持つ。
    void SetSpriteSheet(std::shared_ptr<const SpriteSheet> sheet);
    const std::shared_ptr<const SpriteSheet>& Sprite() const { return m_spriteSheet; }
    virtual FrameAnimation& Frames() { return m_frames; }
    virtual const FrameAnimation& Frames() const { return m_frames; }
    void UpdateFrames(float deltaTime) { Frames().Update(deltaTime); }
    bool DrawAnimation(Render& render, const Rect& bounds, float angle = 0.0f,
        bool flipX = false, unsigned int color = FrameTint, int alpha = FrameOpacity) const;
    bool DrawAnimationFrame(Render& render, const Rect& bounds, int frame, float angle = 0.0f,
        bool flipX = false, unsigned int color = FrameTint, int alpha = FrameOpacity) const;

    static constexpr unsigned int FrameTint = 0xffffff;
    static constexpr int FrameOpacity = 255;

    const Collider& Collision() const { return m_collider; }
    const Rect& GetCollision() const { return m_collider.Bounds(); }
    Rect InterpolatedBounds(const Rect& previous, float alpha) const;
    bool Active() const { return m_active; }

    float X() const { return m_x; }
    float Y() const { return m_y; }

    int Width() const { return m_width; }
    int Height() const { return m_height; }

protected:
    GameObject(int width, int height);

    // 位置は中心座標。右がXの正方向、下がYの正方向。
    float m_x = 0.0f;
    float m_y = 0.0f;
    bool m_active = true;

private:
    int m_width;
    int m_height;
    Collider m_collider;
    std::shared_ptr<const SpriteSheet> m_spriteSheet;
    FrameAnimation m_frames;
};

}
