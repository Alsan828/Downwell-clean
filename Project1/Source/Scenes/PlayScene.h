#pragma once

#include "Scene.h"
#include "PlaySimulation.h"
#include "../Objects/Player.h"
#include "../Objects/Bullets/BulletSystem.h"
#include "../Stage/Stage.h"

namespace downwell {

class SoundManager;

// 主人公とステージを持ち、ゲーム中の処理をまとめる画面。
class PlayScene final : public Scene {
public:
    explicit PlayScene(const InputManager& input, SoundManager* sound = nullptr);

    void Resize(int width, int height) override;
    SceneTransition Update(float deltaTime) override;
    void Draw(Render& render) const override;

private:
    void GenerateStage();
    void LoadSprites();
    void UpdateCamera(float deltaTime);
    void UpdateScreenShake(float deltaTime);
    void ResetScreenShake();
    void PlaySounds();

    enum class State { Playing, Dying, Fading };

    // 音声の寿命はGameが管理する。
    SoundManager* m_sound;
    Player m_player;
    Stage m_stage;
    BulletSystem m_bullets;
    PlaySimulation m_simulation;
    float m_cameraY = 0.0f;
    float m_cameraSpeed = 0.0f;
    float m_shakeTime = 0.0f;
    float m_shakeDuration = 0.0f;
    float m_shakeAmplitude = 0.0f;
    float m_shakeX = 0.0f;
    float m_shakeY = 0.0f;
    State m_state = State::Playing;
    double m_fadeTime = 0.0;
    bool m_gameOver = false;
    int m_stageNumber = FirstStage;

    static constexpr int FirstStage = 1;
    static constexpr int StageIncrement = 1;
    static constexpr float CameraSmoothTime = 0.4f;
    static constexpr float CameraPlayerRatio = 0.55f;
    static constexpr double FadeDuration = 0.8;
    static constexpr double BlackHoldDuration = 0.12;
    static constexpr float ShotShakeDuration = 0.10f;
    static constexpr float ShotShakeAmplitude = 2.5f;
    static constexpr float StompShakeDuration = 0.14f;
    static constexpr float StompShakeAmplitude = 4.0f;
    static constexpr float HurtShakeDuration = 0.18f;
    static constexpr float HurtShakeAmplitude = 5.0f;
    static constexpr float ScreenShakeFrequency = 14.0f;
    static constexpr float ScreenShakeHorizontalRatio = 0.4f;
    static constexpr float ScreenShakeFullTurn = 6.283185307f;
    static constexpr float ScreenShakeTimeTolerance = 1.0e-6f;
};

}
