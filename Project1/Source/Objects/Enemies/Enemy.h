#pragma once

#include "../GameObject.h"
#include "EnemyAI.h"

#include <memory>
#include <vector>

namespace downwell {

struct EnemySprite {
    std::shared_ptr<const SpriteSheet> sheet;
    AnimationClip clip;
    float scale = 1.0f;
    bool facesRight = true;

    bool Valid() const;
};

// 敵が共有する行動、被弾処理、踏み付けの可否。
class Enemy : public GameObject {
public:
    virtual void Update(float deltaTime, const BlockList& blocks = {}, const GameObject* target = nullptr);
    void Draw(Render& render) const override;

    void Hit(int damage = DefaultDamage);
    virtual void HitByBullet(int damage = DefaultDamage);
    void Kill();
    int Health() const { return m_health; }
    virtual bool CanStomp() const { return m_stompable; }
    virtual bool Filled() const { return false; }
    const Rect& PreviousCollision() const { return m_previousCollision; }
    bool SetAnimationSprite(const EnemySprite& sprite);
    Rect AnimationBounds(const Rect& bounds) const;
    float AnimationAngle() const;
    bool AnimationFlipX() const;

    static constexpr int Size = 25;
    static constexpr int DefaultDamage = 1;
    static constexpr int DefaultHealth = 2;
    static constexpr unsigned int Color = 0xff4040;

protected:
    Enemy(float x, float y, bool stompable, int health, int width = Size, int height = Size);

    EnemyAI m_ai;
    bool m_stompable;

private:
    int m_health;
    Rect m_previousCollision{};
    float m_spriteScale = 0.0f;
    bool m_spriteFacesRight = true;
    bool m_facingRight = true;
};

using EnemyList = std::vector<std::unique_ptr<Enemy>>;

}
