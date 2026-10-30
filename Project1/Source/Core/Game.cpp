#include "Game.h"
#include "../Scenes/TitleScene.h"
#include "../Stage/StageData.h"

#include <DxLib.h>
#include <stdexcept>
#include <algorithm>
#include <chrono>

namespace downwell {

namespace {

constexpr float MaxDeltaTime = 0.05f;
constexpr wchar_t BgmRelativePath[] = L"audio/music/bitwise_chase.wav";
constexpr int BgmVolume = 160;
constexpr int EffectsVolume = 180;
constexpr float LandEffectGain = 0.75f;
constexpr float HitEffectGain = 0.85f;
constexpr float HurtEffectGain = 0.85f;

struct EffectAsset {
    SoundEffect effect;
    const wchar_t* path;
    float gain = SoundManager::DefaultSEGain;
};

constexpr EffectAsset EffectAssets[] = {
    { SoundEffect::Shoot, L"audio/effects/shoot.wav" },
    { SoundEffect::Jump, L"audio/effects/jump.wav" },
    { SoundEffect::Land, L"audio/effects/land.wav", LandEffectGain },
    { SoundEffect::BreakBlock, L"audio/effects/break_block.wav", HitEffectGain  },
    { SoundEffect::Stomp, L"audio/effects/stomp.wav" },
    { SoundEffect::Hurt, L"audio/effects/hurt.wav", HurtEffectGain },
    { SoundEffect::Reload, L"audio/effects/reload.wav" }
};

}

int Game::Run() {
    // 描画の準備をして、タイトル画面から開始する。
    m_renderer.Initialize();
    m_scenes.ChangeScene(std::make_unique<TitleScene>(m_input));

    // BGMは起動時に一度だけ始め、ステージが変わっても再生を続ける。
    const auto dataPath = StageData::DefaultPath().parent_path();
    m_renderer.Ui().LoadSprites(dataPath);
    m_sound.SetBgmVolume(BgmVolume);
    m_sound.SetSEVolume(EffectsVolume);
    if (m_sound.LoadBgm(dataPath / BgmRelativePath)) m_sound.PlayBgm();

    // 読み込めなかった音は鳴らさず、ゲームはそのまま続ける。
    for (const auto& asset : EffectAssets) {
        m_sound.LoadSE(asset.effect, dataPath / asset.path, asset.gain);
    }

    auto lastFrame = std::chrono::steady_clock::now();

    // DxLibにウィンドウメッセージを処理させ、閉じる操作かEscで終了する。
    while (ProcessMessage() == 0) {
        const auto now = std::chrono::steady_clock::now();
        // 中断後の大きな時間差で、主人公が一気に移動するのを防ぐ。
        const float deltaTime = std::min(
            std::chrono::duration<float>(now - lastFrame).count(), MaxDeltaTime);
        lastFrame = now;

        // 画面を更新する前に、このフレームの入力を読み取る。
        m_input.Update();

        if (m_input.Pressed(Action::Exit)) break;

        int width = 0, height = 0;
        if (GetWindowSize(&width, &height) < 0) throw std::runtime_error("GetWindowSize failed");

        if (width <= 0 || height <= 0 || IsIconic(GetMainWindowHandle())) {
            // 最小化中は描画を休み、待機なしのループによるCPU負荷を避ける。
            WaitTimer(10);
            continue;
        }

        if (width != m_renderer.Width() || height != m_renderer.Height()) {
            m_renderer.Resize(width, height);
            m_scenes.Resize(width, height);
        }

        m_scenes.Update(deltaTime);
        m_scenes.Draw(m_renderer);
    }

    // 画面内の画像と音声は、DxLibを終了する前に解放する。
    m_scenes.ChangeScene(nullptr);
    m_sound.Clear();
    m_renderer.Shutdown();
    return 0;
}

}
