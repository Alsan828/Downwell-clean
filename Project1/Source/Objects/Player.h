#pragma once

#include "GameObject.h"
#include "../Graphics/Animation.h"
#include "../Graphics/SpriteEffects.h"
#include "../Stage/Blocks/Block.h"
#include "../Physics/Gravity.h"

#include <array>
#include <vector>

namespace downwell {

class InputManager;

// 入力による移動、ジャンプ、地形との衝突を処理する。
class Player final : public GameObject {
public:
    enum class InputAction { None, Jump, Shoot };
    enum class Event { Jump, Land, Shoot, Reload, Stomp, Hurt };

    Player();

    void Reset(int width, int height);
    void Reset(int width, int height, float x, float y);
    void Resize(int width, int height, const BlockList& blocks);

    // 入力の意図と、自分自身の物理更新を分ける。
    void ClearFrameEvents();
    InputAction ReadInput(const InputManager& input, const BlockList& blocks);
    void BeginPhysicsStep(float deltaTime);
    void UpdatePhysics(float deltaTime, BlockList& blocks);
    void UpdateAnimation(float deltaTime);
    void SetInterpolationAlpha(float alpha);

    // 接触相手や弾の管理はせず、自分の反応だけを処理する。
    bool CanShoot() const;
    void ApplyShot(float fallSpeed);
    void NotifyReload();
    void BounceFromStomp(float enemyTop);
    bool TakeHit(float sourceX);

    void Draw(Render& render) const override;
    void BeginDeath();
    float UpdateDeath(float deltaTime);
    bool DeathFinished() const;
    bool DeathVisible() const;

    bool Grounded() const { return m_grounded; }
    bool Landed() const { return m_landed; }
    bool Dying() const { return m_dying; }
    float VerticalSpeed() const { return m_verticalSpeed; }
    float DrawAngle() const { return m_drawAngle; }
    Rect DrawBounds() const;
    float DrawY() const;
    float InterpolationAlpha() const;
    bool Hurt() const { return m_hurtTime > 0.0f; }
    int Health() const { return m_health; }
    // 新しいゲームを始めるときだけ体力を戻す。
    void ResetHealth() { m_health = MaxHealth; }
    const std::vector<Event>& Events() const { return m_events; }
    const std::vector<SpriteEffectEvent>& SpriteEvents() const { return m_spriteEvents; }
    const Animation& CurrentAnimation() const { return m_animation; }
    FrameAnimation& Frames() override { return m_animation.Frames(); }
    const FrameAnimation& Frames() const override { return m_animation.Frames(); }
    void SetAnimationClip(AnimationState state, const AnimationClip& clip) { m_animation.SetClip(state, clip); }
    bool SetAnimationSprite(AnimationState state, std::shared_ptr<const SpriteSheet> sheet,
        const AnimationClip& clip, float scale = 1.0f, bool facesRight = true);
    bool FacingRight() const { return m_facingRight; }

    static constexpr int Size = 25;
    static constexpr int MaxHealth = 4;
    static constexpr int MinHealth = 0;
    static constexpr int HitDamage = 1;
    static constexpr float DeathAnimationDuration = 0.36f;
    static constexpr float DeathBlinkDuration = 1.0f;
    static constexpr float DeathFlashInterval = 0.1f;
    static constexpr float DeathDuration = DeathAnimationDuration + DeathBlinkDuration;

private:
    void ClearJumpBrake();
    void StopHorizontalMotion();
    void ClampPosition();
    void StartJump(int direction, bool wallJump);
    bool CanWallJump(int direction, const BlockList& blocks) const;
    void StepMovement(int direction, float deltaTime, BlockList& blocks);
    void StepVertical(int direction, float deltaTime, BlockList& blocks);
    float ResolveLanding(float previousY, float nextY, const BlockList& blocks);
    bool HasSupport(const BlockList& blocks) const;

    int m_moveDirection = 0;
    float m_horizontalSpeed = 0.0f;
    bool m_wallJumpDrift = false;
    int m_windowWidth = 800;
    int m_windowHeight = 600;
    float m_verticalSpeed = 0.0f;
    float m_jumpBrakeAcceleration = 0.0f;
    float m_jumpBrakeTime = 0.0f;
    // 見た目だけの回転。位置や当たり判定には使わない。
    float m_drawAngle = 0.0f;
    float m_previousDrawAngle = 0.0f;
    Rect m_previousDrawBounds{};
    int m_rotationDirection = 0;
    float m_interpolationAlpha = 0.0f;
    Animation m_animation;
    struct StateSprite {
        std::shared_ptr<const SpriteSheet> sheet;
        float scale = 1.0f;
        bool facesRight = true;
    };
    std::array<StateSprite, static_cast<std::size_t>(AnimationState::Count)> m_stateSprites{};
    bool m_facingRight = true;
    bool m_grounded = true;
    bool m_landed = false;
    bool m_jumpCut = false;
    bool m_landingHopReady = false;
    bool m_softenApex = false;
    int m_health = MaxHealth;
    float m_hurtTime = 0.0f;
    float m_knockbackTime = 0.0f;
    int m_knockbackDirection = 0;
    std::vector<Event> m_events;
    std::vector<SpriteEffectEvent> m_spriteEvents;
    bool m_dying = false;
    double m_deathTime = 0.0;
    Rect m_deathDrawBounds{};
    float m_deathInterpolationAlpha = 1.0f;
    float m_deathStartAngle = 0.0f;
    float m_deathAngleTravel = 0.0f;

    // 速度はpx/秒、時間は秒単位。平滑化時間が短いほど素早く反応する。
    static constexpr float MoveSpeed = 360.0f;
    static constexpr float MoveSmoothTime = 0.12f;
    static constexpr float StopSmoothTime = 0.025f;
    static constexpr float AnimationMoveThreshold = 1.0f;

    static constexpr float FullTurn = 6.2831853f;
    static constexpr float RotationSpeed = FullTurn * 2.0f;
    static constexpr float DeathQuarterTurn = FullTurn * 0.25f;
    static constexpr int DeathSingleFrame = 1;
    static constexpr int DeathFlashPhases = 2;
    static constexpr double DeathTimeTolerance = 1.0e-7;

    static constexpr float JumpSpeed = 585.0f;
    static constexpr float WallContactTolerance = 0.5f;
    static constexpr float BlockContactTolerance = 0.01f;
    static constexpr float WallJumpDustAngle = FullTurn * 0.25f;
    // 高さを約半分にするため、初速を通常の1/√2にする。
    static constexpr float WallJumpSpeed = JumpSpeed * 0.70710678f;
    static constexpr float WallPushSpeed = 200.0f;
    static constexpr float WallJumpSmoothTime = 0.18f;
    static constexpr float ShortJumpRatio = 0.4f;
    static constexpr float JumpCutDuration = 0.05f;
    Gravity m_riseGravity{ 1500.0f, 900.0f };
    Gravity m_fallGravity{ 1500.0f, 900.0f };
    static constexpr float BreakRecoilSpeed = 220.0f;
    static constexpr float LandingHopSpeed = 120.0f;
    // 着地の小跳びや低速の接触は鳴らさない。
    static constexpr float LandingSoundMinSpeed = 150.0f;
    static constexpr float StompSpeed = 370.0f;
    static constexpr float HurtDuration = 0.8f;
    static constexpr float HurtFlashInterval = 0.08f;
    static constexpr float KnockbackDuration = 0.15f;
    static constexpr float KnockbackSpeed = 360.0f;
    static constexpr float HurtJumpSpeed = 180.0f;
};

}
