#include "ResultScene.h"
#include "../Core/InputManager.h"
#include "../Graphics/Render.h"
#include "../Physics/Collider.h"

#include <algorithm>
#include <string>

namespace downwell {

namespace {

constexpr int MinReachedStage = 0;
constexpr float CenterX = 160.0f;
constexpr float PanelLeft = 24.0f;
constexpr float PanelRight = 296.0f;
constexpr float PanelTop = 158.0f;
constexpr float PanelBottom = 408.0f;
constexpr float HeadingTop = 190.0f;
constexpr float HeadingFontHeight = 28.0f;
constexpr float AccentHalfWidth = 90.0f;
constexpr float AccentTop = 238.0f;
constexpr float AccentHeight = 3.0f;
constexpr float StageTop = 278.0f;
constexpr float StageFontHeight = 18.0f;
constexpr float PromptTop = 348.0f;
constexpr float PromptFontHeight = 13.0f;
constexpr unsigned int TextColor = 0xffffff;
constexpr unsigned int AccentColor = 0xff0000;
constexpr wchar_t HeadingText[] = L"GAME OVER";
constexpr wchar_t ReachedStageText[] = L"REACHED STAGE ";
constexpr wchar_t ReturnText[] = L"SPACE / PAD A : TITLE";

}

ResultScene::ResultScene(const InputManager& input, int reachedStage)
    : Scene(input), m_reachedStage(std::max(MinReachedStage, reachedStage)) {
}

void ResultScene::Resize(int, int) {
    // 描画の倍率はRenderがウィンドウサイズから求める。
}

SceneTransition ResultScene::Update(float) {
    if (m_input.Pressed(Action::JumpShoot)) return { SceneType::Title };
    return {};
}

void ResultScene::Draw(Render& render) const {
    const std::wstring stageText = ReachedStageText + std::to_wstring(m_reachedStage);
    const auto& ui = render.Ui();

    render.BeginScreen();
    const bool panel = ui.DrawMenuPanel({ PanelLeft, PanelTop, PanelRight, PanelBottom });
    ui.DrawScreenText(HeadingText, CenterX, HeadingTop, HeadingFontHeight, AccentColor);
    if (!panel) {
        render.DrawRectangle({ CenterX - AccentHalfWidth, AccentTop,
            CenterX + AccentHalfWidth, AccentTop + AccentHeight }, AccentColor);
    }
    ui.DrawScreenText(stageText.c_str(), CenterX, StageTop, StageFontHeight, TextColor);
    ui.DrawScreenText(ReturnText, CenterX, PromptTop, PromptFontHeight, TextColor);
    render.EndScreen();
}

}
