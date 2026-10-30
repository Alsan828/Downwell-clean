#include "PlayScene.h"
#include "../Graphics/Render.h"
#include "../Graphics/SpriteSheet.h"
#include "../Audio/SoundManager.h"

#include <DxLib.h>
#include <algorithm>
#include <cmath>
#include <optional>

namespace downwell {

namespace {

constexpr wchar_t KnightSpritePath[] = L"sprites/gloomy_knight/knightAtlas.png";
constexpr wchar_t CaveSpritePath[] = L"sprites/monochrome_caves/bw_tiles.png";
constexpr int KnightFrameSize = 16;
constexpr int CaveFrameSize = 8;
constexpr float KnightScale = 2.0f;
constexpr float KnightFrameDuration = 0.12f;
constexpr int KnightIdleFirstFrame = 0;
constexpr int KnightIdleFrameCount = 2;
constexpr int KnightWalkFirstFrame = 4;
constexpr int KnightWalkFrameCount = 4;
constexpr int KnightJumpFrame = 8;
constexpr int KnightFallFrame = 9;
constexpr int SingleFrame = 1;
constexpr int CaveFixedFrame = 47;
constexpr int CaveBreakableFrame = 32;
constexpr int CaveWallFrame = 42;
constexpr wchar_t SlimeSpritePath[] = L"sprites/grotto_escape/enemies.png";
constexpr wchar_t BatSpritePath[] = L"sprites/chibi_monsters/bat.png";
constexpr wchar_t FrogSpritePath[] = L"sprites/chibi_monsters/frog.png";
constexpr int EnemyFrameSize = 16;
constexpr int EnemyFirstFrame = 0;
constexpr int SlimeFrameCount = 3;
constexpr int CreatureFrameCount = 3;
constexpr float EnemySpriteScale = 2.0f;
constexpr float ChibiFrameDuration = 0.1f;
constexpr float SlimeFrameDuration = 0.2f;
constexpr bool SlimeFacesRight = true;
constexpr bool CreatureFacesRight = true;
constexpr wchar_t BulletSpritePath[] = L"sprites/pixel_art_projectiles/ricochet_bullet.png";
constexpr int BulletFrameWidth = 12;
constexpr int BulletFrameHeight = 3;
constexpr wchar_t HitSpritePath[] = L"sprites/warped_shooting_fx/hit-1.png";
constexpr wchar_t StompSpritePath[] = L"sprites/warped_shooting_fx/hit-2.png";
constexpr wchar_t ExplosionSpritePath[] = L"sprites/explosion_animations/explosion-a.png";
constexpr wchar_t DustSpritePath[] = L"sprites/dust_animation/dust.png";
constexpr int HitFrameSize = 32;
constexpr int HitFrameCount = 5;
constexpr int StompFrameCount = 7;
constexpr int ExplosionFrameSize = 32;
constexpr int ExplosionFrameCount = 8;
constexpr int DustFrameWidth = 10;
constexpr int DustFrameHeight = 7;
constexpr int DustFrameCount = 8;
constexpr float HitFrameDuration = 0.04f;
constexpr float ExplosionFrameDuration = 0.05f;
constexpr float DustFrameDuration = 0.04f;
constexpr float HitScale = 1.0f;
constexpr float ExplosionScale = 2.0f;
constexpr float DustScale = 2.0f;

std::optional<SoundEffect> SoundFor(Player::Event event) {
    switch (event) {
    case Player::Event::Jump: return SoundEffect::Jump;
    case Player::Event::Land: return SoundEffect::Land;
    case Player::Event::Shoot: return SoundEffect::Shoot;
    case Player::Event::Reload: return SoundEffect::Reload;
    case Player::Event::Stomp: return SoundEffect::Stomp;
    case Player::Event::Hurt: return SoundEffect::Hurt;
    }
    return std::nullopt;
}

void LoadEffect(SpriteEffects& effects, SpriteEffectType type, const std::filesystem::path& file,
    int width, int height, int frameCount, float duration, float scale, bool bottomAligned = false) {
    auto sheet = std::make_shared<SpriteSheet>();
    if (sheet->Load(file, width, height)) {
        effects.SetSprite(type, sheet, { 0, frameCount, duration, false }, scale, bottomAligned);
    }
}

EnemySprite LoadEnemySprite(const std::filesystem::path& file, int frameCount, bool facesRight,
    float frameDuration = ChibiFrameDuration) {
    auto sheet = std::make_shared<SpriteSheet>();
    if (!sheet->Load(file, EnemyFrameSize, EnemyFrameSize)) return {};
    return { sheet, { EnemyFirstFrame, frameCount, frameDuration, true }, EnemySpriteScale, facesRight };
}

struct KnightClip {
    AnimationState state;
    int firstFrame;
    int frameCount;
    float frameDuration = KnightFrameDuration;
    bool loop = true;
};

constexpr KnightClip KnightClips[] = {
    { AnimationState::Idle, KnightIdleFirstFrame, KnightIdleFrameCount },
    { AnimationState::Move, KnightWalkFirstFrame, KnightWalkFrameCount },
    { AnimationState::Jump, KnightJumpFrame, SingleFrame },
    { AnimationState::Fall, KnightFallFrame, SingleFrame },
    // 被弾専用のコマはないため、待機の絵と従来の赤い点滅を使う。
    { AnimationState::Hurt, KnightIdleFirstFrame, KnightIdleFrameCount },
    // 死亡用のコマがないため、落下の絵を使って倒れる演出をする。
    { AnimationState::Death, KnightFallFrame, SingleFrame, Player::DeathAnimationDuration, false }
};

}

PlayScene::PlayScene(const InputManager& input, SoundManager* sound)
    : Scene(input), m_sound(sound) {
    LoadSprites();
    GenerateStage();
}

void PlayScene::LoadSprites() {
    if (!DxLib_IsInit()) return;

    const auto dataRoot = StageData::DefaultPath().parent_path();
    auto bullet = std::make_shared<SpriteSheet>();
    if (bullet->Load(dataRoot / BulletSpritePath, BulletFrameWidth, BulletFrameHeight)) {
        m_bullets.SetBulletSprite(bullet);
    }
    auto knight = std::make_shared<SpriteSheet>();
    if (knight->Load(dataRoot / KnightSpritePath, KnightFrameSize, KnightFrameSize)) {
        for (const auto& clip : KnightClips) {
            m_player.SetAnimationSprite(clip.state, knight,
                { clip.firstFrame, clip.frameCount, clip.frameDuration, clip.loop }, KnightScale);
        }
    }

    // 同じタイル画像を全ブロックで共有し、再生成後も引き継ぐ。
    auto cave = std::make_shared<SpriteSheet>();
    if (cave->Load(dataRoot / CaveSpritePath, CaveFrameSize, CaveFrameSize)) {
        m_stage.SetBlockSprites(cave, CaveFixedFrame, CaveBreakableFrame, CaveWallFrame);
    }

    // 敵画像は次のステージでも共有する。スライムは同じ作者の別素材集を使う。
    m_stage.SetEnemySprites(
        LoadEnemySprite(dataRoot / SlimeSpritePath, SlimeFrameCount, SlimeFacesRight, SlimeFrameDuration),
        LoadEnemySprite(dataRoot / BatSpritePath, CreatureFrameCount, CreatureFacesRight),
        LoadEnemySprite(dataRoot / FrogSpritePath, CreatureFrameCount, CreatureFacesRight));

    // 接触位置に一度だけエフェクトを出す。
    auto& effects = m_stage.Effects();
    LoadEffect(effects, SpriteEffectType::BulletHit, dataRoot / HitSpritePath,
        HitFrameSize, HitFrameSize, HitFrameCount, HitFrameDuration, HitScale);
    LoadEffect(effects, SpriteEffectType::Stomp, dataRoot / StompSpritePath,
        HitFrameSize, HitFrameSize, StompFrameCount, HitFrameDuration, HitScale);
    LoadEffect(effects, SpriteEffectType::EnemyDisappear, dataRoot / ExplosionSpritePath,
        ExplosionFrameSize, ExplosionFrameSize, ExplosionFrameCount, ExplosionFrameDuration, ExplosionScale);
    LoadEffect(effects, SpriteEffectType::Dust, dataRoot / DustSpritePath,
        DustFrameWidth, DustFrameHeight, DustFrameCount, DustFrameDuration, DustScale, true);
}

void PlayScene::GenerateStage() {
    // 同じStageを使い、新しい区画・敵・演出に入れ替える。
    m_stage.Generate(Stage::WellWidth);
    m_bullets.Reset();
    // 新しい主人公の位置へ、前の物理刻みや連射を持ち越さない。
    m_simulation.Reset();
    // 体力は次のステージへ引き継ぐ。新しいゲームではPlayerの初期値を使う。
    m_gameOver = false;
    // 井戸の上側中央から入り、重力で自然に落下する。
    m_player.Reset(Stage::WellWidth, Stage::ViewHeight,
        Stage::WellWidth * 0.5f, -Player::Size * 0.5f);
    m_cameraY = 0.0f;
    m_cameraSpeed = 0.0f;
    ResetScreenShake();
    m_fadeTime = 0.0;
    m_state = State::Playing;
}

void PlayScene::Resize(int, int) {
    // 表示倍率はRenderが調整する。区画と主人公のワールド座標は変えない。
}

SceneTransition PlayScene::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return {};

    if (m_state == State::Dying) {
        // 死亡演出だけを進め、消えた後に従来のフェードへつなぐ。
        deltaTime = m_player.UpdateDeath(deltaTime);
        if (!m_player.DeathFinished()) return {};

        m_state = State::Fading;
        m_fadeTime = 0.0;
    }

    if (m_state == State::Fading) {
        // 演出中は位置を固定し、全体が消えてから次のステージへ進む。
        m_fadeTime += deltaTime;
        if (m_fadeTime >= FadeDuration + BlackHoldDuration) {
            if (m_gameOver) return { SceneType::Result, m_stageNumber };

            m_stageNumber += StageIncrement;
            GenerateStage();
        }
        return {};
    }

    m_simulation.Update(m_player, m_input, deltaTime, m_stage.Blocks(), &m_stage.Enemies(), &m_bullets);
    UpdateScreenShake(deltaTime);
    PlaySounds();
    // 衝突処理が終わってから、ブロックの削除と破壊演出を更新する。
    m_stage.Update(deltaTime);
    // 更新内で複数回接触しても、その時点の座標から演出を始める。
    for (const auto& event : m_player.SpriteEvents()) m_stage.Effects().Add(event);
    for (const auto& event : m_bullets.ImpactEvents()) m_stage.Effects().Add(event);

    UpdateCamera(deltaTime);

    if (m_player.Health() == Player::MinHealth) {
        m_gameOver = true;
        m_player.BeginDeath();
        m_state = State::Dying;
        m_fadeTime = 0.0;
        ResetScreenShake();
    } else if (m_player.GetCollision().top >= m_stage.Height()) {
        // 通常のクリアは、死亡演出を挟まず次のステージへ進む。
        m_state = State::Fading;
        m_fadeTime = 0.0;
        ResetScreenShake();
    }

    return {};
}

void PlayScene::UpdateScreenShake(float deltaTime) {
    m_shakeTime = std::max(0.0f, m_shakeTime - deltaTime);
    if (m_shakeTime <= ScreenShakeTimeTolerance) ResetScreenShake();

    float duration = 0.0f;
    float strength = 0.0f;
    for (const auto event : m_player.Events()) {
        float eventDuration = 0.0f;
        float eventStrength = 0.0f;
        switch (event) {
        case Player::Event::Shoot:
            eventDuration = ShotShakeDuration;
            eventStrength = ShotShakeAmplitude;
            break;
        case Player::Event::Stomp:
            eventDuration = StompShakeDuration;
            eventStrength = StompShakeAmplitude;
            break;
        case Player::Event::Hurt:
            eventDuration = HurtShakeDuration;
            eventStrength = HurtShakeAmplitude;
            break;
        default:
            break;
        }
        if (eventStrength > strength) {
            duration = eventDuration;
            strength = eventStrength;
        }
    }

    // 強い反応を優先し、発射で踏み付けや被弾の揺れを弱めない。
    if (strength > 0.0f && strength >= m_shakeAmplitude) {
        m_shakeDuration = duration;
        m_shakeAmplitude = strength;
        m_shakeTime = duration;
    }
    if (m_shakeTime <= 0.0f) return;

    const float remaining = m_shakeTime / m_shakeDuration;
    const float amplitude = m_shakeAmplitude * remaining * remaining;
    const float phase = (m_shakeDuration - m_shakeTime) * ScreenShakeFrequency * ScreenShakeFullTurn;

    // 描画だけを短く揺らし、時間とともに滑らかに収める。
    m_shakeX = std::sin(phase) * amplitude * ScreenShakeHorizontalRatio;
    m_shakeY = -std::cos(phase) * amplitude;
}

void PlayScene::ResetScreenShake() {
    m_shakeTime = 0.0f;
    m_shakeDuration = 0.0f;
    m_shakeAmplitude = 0.0f;
    m_shakeX = 0.0f;
    m_shakeY = 0.0f;
}

void PlayScene::PlaySounds() {
    if (!m_sound) return;

    for (const auto event : m_player.Events()) {
        if (const auto effect = SoundFor(event)) m_sound->PlaySE(*effect);
    }

    // 削除する前に確認し、一度に複数壊れても破壊音は一回だけ鳴らす。
    const auto& blocks = m_stage.Blocks();
    const bool broken = std::any_of(blocks.begin(), blocks.end(), [](const auto& block) {
        return block->Destroyed() && block->IsBreakable();
    });
    if (broken) m_sound->PlaySE(SoundEffect::BreakBlock);
}

void PlayScene::UpdateCamera(float deltaTime) {
    const float maxCameraY = static_cast<float>(std::max(0, m_stage.Height() - Stage::ViewHeight));
    const float damping = 2.0f / CameraSmoothTime;
    const float fallSpeed = std::max(0.0f, m_player.VerticalSpeed());

    // 実際の落下速度を先読みし、高速でも中央より少し下に保つ。
    const float lookAheadTime = std::max(0.0f, CameraSmoothTime - deltaTime * 0.5f);
    const float target = std::clamp(
        m_player.DrawY() + fallSpeed * lookAheadTime - Stage::ViewHeight * CameraPlayerRatio,
        m_cameraY, maxCameraY);

    // 位置と速度を一緒に減衰させ、着地や上昇でも急停止させない。
    const float distance = m_cameraY - target;
    const float change = (m_cameraSpeed + damping * distance) * deltaTime;
    const float decay = std::exp(-damping * deltaTime);
    const float nextY = target + (distance + change) * decay;

    m_cameraSpeed = std::max(0.0f, (m_cameraSpeed - damping * change) * decay);
    m_cameraY = std::clamp(nextY, m_cameraY, maxCameraY);
    if (m_cameraY >= maxCameraY) m_cameraSpeed = 0.0f;
}

void PlayScene::Draw(Render& render) const {
    const float fade = static_cast<float>(std::min(m_fadeTime / FadeDuration, 1.0));
    render.Draw(m_player, m_stage, m_cameraY, Stage::ViewHeight, fade, &m_bullets,
        m_player.InterpolationAlpha(), &m_player, m_shakeX, m_shakeY);
}

}
