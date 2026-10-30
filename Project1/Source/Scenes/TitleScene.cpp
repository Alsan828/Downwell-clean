#include "TitleScene.h"
#include "../Core/InputManager.h"
#include "../Graphics/Render.h"
#include "../Physics/Collider.h"

#include <cmath>

namespace downwell {

namespace {

constexpr float CenterX = 160.0f;
constexpr float PlayerHalfSize = 12.5f;
constexpr float PlayerCenterY = 176.0f;
constexpr float KnightHalfSize = 16.0f;
constexpr float KnightFrameDuration = 0.12f;
constexpr int KnightIdleFrameCount = 2;
constexpr float PanelLeft = 24.0f;
constexpr float PanelRight = 296.0f;
constexpr float PanelTop = 120.0f;
constexpr float PanelBottom = 484.0f;
constexpr float TitleTop = 232.0f;
constexpr float TitleFontHeight = 32.0f;
constexpr float AccentHalfWidth = 90.0f;
constexpr float AccentTop = 284.0f;
constexpr float AccentHeight = 3.0f;
constexpr float PromptTop = 332.0f;
constexpr float PromptFontHeight = 13.0f;
constexpr float MoveHelpTop = 414.0f;
constexpr float JumpHelpTop = 438.0f;
constexpr float HelpFontHeight = 12.0f;
constexpr float BlinkPeriod = 1.2f;
constexpr float BlinkVisibleTime = 0.8f;
constexpr unsigned int TextColor = 0xffffff;
constexpr unsigned int AccentColor = 0xff0000;
constexpr unsigned int HelpColor = 0x999999;
constexpr wchar_t TitleText[] = L"DOWNWELL";
constexpr wchar_t StartText[] = L"SPACE / PAD A : START";
constexpr wchar_t MoveHelpText[] = L"A/D / STICK / D-PAD : MOVE";
constexpr wchar_t JumpHelpText[] = L"SPACE / PAD A : JUMP / SHOOT";

}

TitleScene::TitleScene(const InputManager& input) : Scene(input) {
}

void TitleScene::Resize(int, int) {
    // 描画の倍率はRenderがウィンドウサイズから求める。
}

SceneTransition TitleScene::Update(float deltaTime) {
    if (deltaTime > 0.0f) m_blinkTime = std::fmod(m_blinkTime + deltaTime, BlinkPeriod);

    // 押した瞬間だけ開始し、長押しでは繰り返さない。
    if (m_input.Pressed(Action::JumpShoot)) return { SceneType::Play };
    return {};
}

void TitleScene::Draw(Render& render) const {
    const auto& ui = render.Ui();
    render.BeginScreen();
    const bool panel = ui.DrawMenuPanel({ PanelLeft, PanelTop, PanelRight, PanelBottom });
    const int frame = static_cast<int>(m_blinkTime / KnightFrameDuration) % KnightIdleFrameCount;
    if (!ui.DrawMenuCharacter({ CenterX - KnightHalfSize, PlayerCenterY - KnightHalfSize,
        CenterX + KnightHalfSize, PlayerCenterY + KnightHalfSize }, frame)) {
        render.DrawRectangle({ CenterX - PlayerHalfSize, PlayerCenterY - PlayerHalfSize,
            CenterX + PlayerHalfSize, PlayerCenterY + PlayerHalfSize }, TextColor);
    }
    ui.DrawScreenText(TitleText, CenterX, TitleTop, TitleFontHeight, TextColor);
    if (!panel) {
        render.DrawRectangle({ CenterX - AccentHalfWidth, AccentTop,
            CenterX + AccentHalfWidth, AccentTop + AccentHeight }, AccentColor);
    }

    if (m_blinkTime < BlinkVisibleTime)
        ui.DrawScreenText(StartText, CenterX, PromptTop, PromptFontHeight, TextColor);

    ui.DrawScreenText(MoveHelpText, CenterX, MoveHelpTop, HelpFontHeight, HelpColor);
    ui.DrawScreenText(JumpHelpText, CenterX, JumpHelpTop, HelpFontHeight, HelpColor);
    render.EndScreen();
}

}
