#pragma once

#include "../../Stage/Blocks/Block.h"
#include "../../Physics/PhysicsTiming.h"

namespace downwell {

class Enemy;

// 地形に沿う移動、平面の巡回、主人公を追う浮遊行動を管理する。
class EnemyAI final {
public:
    void SetChasing();
    void SetCrawling(int direction);
    void SetWalking(int direction);
    void ApplyBulletKnockback();
    void Update(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target);
    // 描画から向きと再生時間を読み取り、速度や判定は変更しない。
    int AnimationDirection() const;
    float AnimationAngle() const;
    bool AnimationResting() const;
    float AnimationDeltaTime() const { return static_cast<float>(m_animationDeltaTime); }

    static constexpr float CrawlSpeed = 48.0f;
    static constexpr double WalkMinPhaseDuration = 4.0;
    static constexpr double WalkMaxPhaseDuration = 6.0;
    static constexpr float ChaseSpeed = 60.0f;
    static constexpr float ChaseSmoothTime = 1.2f;
    static constexpr float CeilingBounceDuration = 0.625f;
    static constexpr float BulletKnockbackSpeed = 180.0f;
    static constexpr float BulletKnockbackDecayTime = 0.18f;

private:
    enum class State { Idle, Chasing, Crawling, Walking };

    void UpdateMovement(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target);
    void StepCrawling(Enemy& enemy, float deltaTime, const BlockList& blocks);
    void StepWalking(Enemy& enemy, double deltaTime, const BlockList& blocks);
    bool StepWalkingPhase(Enemy& enemy, float deltaTime, const BlockList& blocks, float floorLeft, float floorRight);
    static double NextWalkPhaseDuration();
    void StepChasing(Enemy& enemy, float deltaTime, const BlockList& blocks, const GameObject* target);
    void Fall(Enemy& enemy, float deltaTime, const BlockList& blocks);

    State m_state = State::Idle;
    double m_physicsAccumulator = 0.0;
    double m_animationDeltaTime = 0.0;
    bool m_walking = true;
    double m_walkTimeRemaining = 0.0;
    int m_surface = 0;
    int m_crawlDirection = 1;
    bool m_attached = true;
    float m_fallSpeed = 0.0f;
    float m_velocityX = 0.0f;
    float m_velocityY = 0.0f;
    float m_bulletKnockbackSpeed = 0.0f;
    float m_bounceTime = 0.0f;
    float m_bounceOriginY = 0.0f;
    bool m_bouncing = false;

    static constexpr double PhysicsStep = physics::Step;
    // 切り替え時の小さな時間の丸め誤差を吸収する。
    static constexpr double WalkPhaseTolerance = 1.0e-9;
    static constexpr float FallAcceleration = 1500.0f;
    static constexpr float MaxFallSpeed = 900.0f;
    static constexpr float BulletKnockbackStopSpeed = 0.01f;
    static constexpr float SurfaceQuarterTurn = 1.570796327f;
};

}
