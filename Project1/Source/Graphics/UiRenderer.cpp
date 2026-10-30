#include "UiRenderer.h"
#include "Render.h"
#include "SpriteSheet.h"
#include "../Objects/Player.h"
#include "../Objects/Bullets/BulletSystem.h"
#include "../Stage/Stage.h"

#include <DxLib.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <stdexcept>

namespace downwell {
namespace {

constexpr float HudMargin = 24.0f;
constexpr float HudCellGap = 4.0f;
constexpr float HealthCellSize = 16.0f;
constexpr float AmmoCellWidth = 16.0f;
constexpr float AmmoCellHeight = 20.0f;
constexpr float HudFontHeight = 14.0f;
constexpr float HudLabelGap = 8.0f;
constexpr float CenterRatio = 0.5f;
constexpr int MinHudPixels = 1;
constexpr int OneCell = 1;
constexpr unsigned int HealthColor = 0xff0000;
constexpr unsigned int AmmoColor = 0xffffff;
constexpr unsigned int EmptyCellColor = 0x242424;
constexpr unsigned int HudTextColor = 0xffffff;
constexpr wchar_t HealthLabel[] = L"HP %d/%d";
constexpr wchar_t AmmoLabel[] = L"AMMO %d/%d";
constexpr int FullOpacity = 255;
constexpr wchar_t HealthUiPath[] = L"sprites/gloomy_knight/healthMeters.png";
constexpr wchar_t MenuUiPath[] = L"sprites/gloomy_knight/menuPopUp.png";
constexpr wchar_t KnightUiPath[] = L"sprites/gloomy_knight/knightAtlas.png";
constexpr int HealthUiWidth = 48;
constexpr int HealthUiHeight = 32;
constexpr int MenuUiWidth = 120;
constexpr int MenuUiHeight = 24;
constexpr int UiImageFrames = 1;
constexpr int KnightUiFrameSize = 16;
constexpr int KnightUiMinFrames = 2;
constexpr float HeartPixelScale = 2.0f;
constexpr SpriteRegion FullHeartRegion{ 0, 0, 16, 16 };
constexpr SpriteRegion EmptyHeartRegion{ 32, 0, 16, 16 };
constexpr SpriteRegion FullAmmoRegion{ 24, 16, 8, 8 };
constexpr SpriteRegion EmptyAmmoRegion{ 24, 24, 8, 8 };
constexpr float AmmoQuarterTurn = 1.57079632679f;
constexpr int MenuPanelIndex = 4;
constexpr int MenuPanelSourceSize = 24;
constexpr int MenuPanelSourceBorder = 6;
constexpr float MenuPanelPixelScale = 2.0f;
constexpr int PanelSections = 3;
constexpr float SkinnedHudMargin = 32.0f;
constexpr float CompactHudMargin = 16.0f;
constexpr float SkinnedHeartGap = 2.0f;
constexpr float HeartVisibleInset = 4.0f;
constexpr float HealthHeaderGap = 6.0f;
constexpr float HudCaptionHeight = 12.0f;
constexpr float HudValueHeight = 18.0f;
constexpr int MinCaptionPixels = 9;
constexpr int MinValuePixels = 12;
constexpr float SkinnedAmmoWidth = 24.0f;
constexpr float SkinnedAmmoHeight = 22.0f;
constexpr float SkinnedAmmoGap = 3.0f;
constexpr float AmmoCaptionGap = 10.0f;
constexpr float AmmoValueGap = 10.0f;
constexpr unsigned int HudCaptionColor = 0x92a8cf;
constexpr unsigned int HudValueColor = 0xebf4ff;
constexpr wchar_t HealthCaption[] = L"HP";
constexpr wchar_t AmmoCaption[] = L"AMMO";
constexpr wchar_t HudValueFormat[] = L"%d / %d";

void Check(int result, const char* operation) {
    if (result < 0) throw std::runtime_error(operation);
}

}

UiRenderer::UiRenderer(Render& render) : m_render(render) {
}

void UiRenderer::Clear() {
    m_healthUi.reset();
    m_menuUi.reset();
    m_knightUi.reset();
}

bool UiRenderer::LoadSprites(const std::filesystem::path& dataRoot) {
    if (!m_render.Initialized() || dataRoot.empty()) return false;

    auto health = std::make_shared<SpriteSheet>();
    auto menu = std::make_shared<SpriteSheet>();
    auto knight = std::make_shared<SpriteSheet>();
    if (!health->Load(dataRoot / HealthUiPath, HealthUiWidth, HealthUiHeight)
        || health->FrameCount() != UiImageFrames
        || !menu->Load(dataRoot / MenuUiPath, MenuUiWidth, MenuUiHeight)
        || menu->FrameCount() != UiImageFrames
        || !knight->Load(dataRoot / KnightUiPath, KnightUiFrameSize, KnightUiFrameSize)
        || knight->FrameCount() < KnightUiMinFrames) return false;

    // 全て読み込めた場合だけ切り替え、失敗時は現在の表示を保つ。
    m_healthUi = std::move(health);
    m_menuUi = std::move(menu);
    m_knightUi = std::move(knight);
    return true;
}

void UiRenderer::DrawHud(const Player& player, const BulletSystem* bullets) const {
    if (m_healthUi) {
        DrawSkinnedHud(player, bullets);
        return;
    }
    const float scale = std::min(static_cast<float>(m_render.Width()) / Stage::WellWidth,
        static_cast<float>(m_render.Height()) / Stage::ViewHeight);
    const auto pixels = [scale](float size) {
        return std::max(MinHudPixels, static_cast<int>(std::lround(size * scale)));
    };
    const int margin = pixels(HudMargin);
    const int gap = pixels(HudCellGap);
    const int healthSize = pixels(HealthCellSize);
    const int fontHeight = pixels(HudFontHeight);
    const int labelGap = pixels(HudLabelGap);
    const int defaultFontSize = GetFontSize();
    if (defaultFontSize <= 0) throw std::runtime_error("GetFontSize failed");
    const double textScale = static_cast<double>(fontHeight) / defaultFontSize;

    // DxLibの標準フォントを拡大描画し、フォント設定は変更しない。
    Check(DrawExtendFormatString(margin, margin, textScale, textScale, HudTextColor,
        HealthLabel, player.Health(), Player::MaxHealth), "Draw health label failed");
    const int healthTop = margin + fontHeight + labelGap;
    for (int cell = 0; cell < Player::MaxHealth; ++cell) {
        const int left = margin + cell * (healthSize + gap);
        const bool full = cell < player.Health();
        Check(DrawBox(left, healthTop, left + healthSize, healthTop + healthSize,
            full ? HealthColor : EmptyCellColor, TRUE), "Draw health cell failed");
    }

    if (!bullets) return;
    const int cellWidth = pixels(AmmoCellWidth);
    const int cellHeight = pixels(AmmoCellHeight);
    const int capacity = bullets->Capacity();
    const int barHeight = capacity * cellHeight + (capacity - OneCell) * gap;
    const int barTop = std::max(margin + fontHeight + labelGap,
        static_cast<int>(std::lround((m_render.Height() - barHeight) * CenterRatio)));
    const int barLeft = m_render.Width() - margin - cellWidth;
    const int textWidth = GetDrawExtendFormatStringWidth(textScale, AmmoLabel,
        bullets->Ammo(), capacity);
    Check(textWidth, "Measure ammo label failed");
    Check(DrawExtendFormatString(std::max(margin, m_render.Width() - margin - textWidth),
        barTop - fontHeight - labelGap, textScale, textScale, HudTextColor,
        AmmoLabel, bullets->Ammo(), capacity), "Draw ammo label failed");

    // 一発につき一格。残弾を下から積み、空いた格は暗く表示する。
    for (int cell = 0; cell < capacity; ++cell) {
        const int top = barTop + (capacity - OneCell - cell) * (cellHeight + gap);
        const bool full = cell < bullets->Ammo();
        Check(DrawBox(barLeft, top, barLeft + cellWidth, top + cellHeight,
            full ? AmmoColor : EmptyCellColor, TRUE), "Draw ammo cell failed");
    }
}

void UiRenderer::DrawSkinnedHud(const Player& player, const BulletSystem* bullets) const {
    const float scale = std::min(static_cast<float>(m_render.Width()) / Stage::WellWidth,
        static_cast<float>(m_render.Height()) / Stage::ViewHeight);
    const auto pixels = [scale](float size) {
        return std::max(MinHudPixels, static_cast<int>(std::lround(size * scale)));
    };
    const int margin = pixels(SkinnedHudMargin);
    const int compactMargin = pixels(CompactHudMargin);
    const int captionHeight = std::max(MinCaptionPixels, pixels(HudCaptionHeight));
    const int valueHeight = std::max(MinValuePixels, pixels(HudValueHeight));
    const int fontSize = GetFontSize();
    if (fontSize <= 0) throw std::runtime_error("GetFontSize failed");
    const double captionScale = static_cast<double>(captionHeight) / fontSize;
    const double valueScale = static_cast<double>(valueHeight) / fontSize;
    const float gutter = std::max(0.0f, (m_render.Width() - Stage::WellWidth * scale) * CenterRatio);

    const int heartSize = pixels(FullHeartRegion.width * HeartPixelScale);
    const int heartGap = pixels(SkinnedHeartGap);
    const int heartWidth = Player::MaxHealth * heartSize + (Player::MaxHealth - OneCell) * heartGap;
    // 横に余白があれば中央へ寄せ、細いウィンドウでは端に収める。
    const int healthLeft = gutter >= heartWidth + margin * 2
        ? static_cast<int>(std::lround((gutter - heartWidth) * CenterRatio)) : compactMargin;
    const int inset = pixels(HeartVisibleInset);
    const int valueWidth = GetDrawExtendFormatStringWidth(valueScale, HudValueFormat,
        player.Health(), Player::MaxHealth);
    Check(valueWidth, "Measure health value failed");
    Check(DrawExtendString(healthLeft + inset, margin + valueHeight - captionHeight,
        captionScale, captionScale, HealthCaption, HudCaptionColor), "Draw health caption failed");
    Check(DrawExtendFormatString(healthLeft + heartWidth - inset - valueWidth, margin,
        valueScale, valueScale, HudValueColor, HudValueFormat, player.Health(), Player::MaxHealth),
        "Draw health value failed");
    const int healthTop = margin + valueHeight + pixels(HealthHeaderGap);
    {
        Render::SpriteBatch sprites(m_render);
        for (int cell = 0; cell < Player::MaxHealth; ++cell) {
            const int left = healthLeft + cell * (heartSize + heartGap);
            const Rect bounds{ static_cast<float>(left), static_cast<float>(healthTop),
                static_cast<float>(left + heartSize), static_cast<float>(healthTop + heartSize) };
            const bool full = cell < player.Health();
            if (!m_render.DrawSpriteRegionPixels(*m_healthUi, full ? FullHeartRegion : EmptyHeartRegion,
                bounds, 0.0f, false, Render::DefaultFrameColor, FullOpacity)) {
                sprites.Restore();
                Check(DrawBox(left, healthTop, left + heartSize, healthTop + heartSize,
                    full ? HealthColor : EmptyCellColor, TRUE), "Draw health cell failed");
            }
        }
    }

    if (!bullets) return;
    const int cellWidth = pixels(SkinnedAmmoWidth);
    const int cellHeight = pixels(SkinnedAmmoHeight);
    const int gap = pixels(SkinnedAmmoGap);
    const int capacity = bullets->Capacity();
    const int barHeight = capacity * cellHeight + (capacity - OneCell) * gap;
    const int labelGap = pixels(AmmoCaptionGap);
    const int valueGap = pixels(AmmoValueGap);
    const int groupHeight = captionHeight + labelGap + barHeight + valueGap + valueHeight;
    const int groupTop = std::max(margin, static_cast<int>(std::lround((m_render.Height() - groupHeight) * CenterRatio)));
    const int captionWidth = GetDrawExtendStringWidth(captionScale, AmmoCaption,
        static_cast<int>(std::wcslen(AmmoCaption)));
    const int ammoValueWidth = GetDrawExtendFormatStringWidth(valueScale, HudValueFormat,
        bullets->Ammo(), capacity);
    Check(captionWidth, "Measure ammo caption failed");
    Check(ammoValueWidth, "Measure ammo value failed");
    const int groupWidth = std::max({ cellWidth, captionWidth, ammoValueWidth });
    const float halfGroup = groupWidth * CenterRatio;
    const float minCenter = std::min(compactMargin + halfGroup, m_render.Width() * CenterRatio);
    const float maxCenter = std::max(minCenter, m_render.Width() - compactMargin - halfGroup);
    const float center = std::clamp(m_render.Width() - gutter * CenterRatio,
        minCenter, maxCenter);
    const int centerX = static_cast<int>(std::lround(center));
    const int barLeft = centerX - static_cast<int>(std::lround(cellWidth * CenterRatio));
    const int barTop = groupTop + captionHeight + labelGap;
    Check(DrawExtendString(centerX - static_cast<int>(std::lround(captionWidth * CenterRatio)), groupTop,
        captionScale, captionScale, AmmoCaption, HudCaptionColor), "Draw ammo caption failed");
    Check(DrawExtendFormatString(centerX - static_cast<int>(std::lround(ammoValueWidth * CenterRatio)),
        barTop + barHeight + valueGap, valueScale, valueScale, HudValueColor,
        HudValueFormat, bullets->Ammo(), capacity), "Draw ammo value failed");

    // 幅を持たせた縦の槽に、残弾を下から一発ずつ並べる。
    Render::SpriteBatch sprites(m_render);
    for (int cell = 0; cell < capacity; ++cell) {
        const int top = barTop + (capacity - OneCell - cell) * (cellHeight + gap);
        const float centerY = top + cellHeight * CenterRatio;
        const Rect bounds{ centerX - cellHeight * CenterRatio, centerY - cellWidth * CenterRatio,
            centerX + cellHeight * CenterRatio, centerY + cellWidth * CenterRatio };
        const bool full = cell < bullets->Ammo();
        if (!m_render.DrawSpriteRegionPixels(*m_healthUi, full ? FullAmmoRegion : EmptyAmmoRegion,
            bounds, AmmoQuarterTurn, false, Render::DefaultFrameColor, FullOpacity)) {
            sprites.Restore();
            Check(DrawBox(barLeft, top, barLeft + cellWidth, top + cellHeight,
                full ? AmmoColor : EmptyCellColor, TRUE), "Draw ammo cell failed");
        }
    }
}

bool UiRenderer::DrawMenuPanel(const Rect& bounds) const {
    if (!m_menuUi || !std::isfinite(bounds.left) || !std::isfinite(bounds.top)
        || !std::isfinite(bounds.right) || !std::isfinite(bounds.bottom)) return false;

    const float width = bounds.right - bounds.left;
    const float height = bounds.bottom - bounds.top;
    if (width <= 0.0f || height <= 0.0f || !std::isfinite(width) || !std::isfinite(height)) return false;
    const float corner = std::min({ MenuPanelSourceBorder * MenuPanelPixelScale,
        width * CenterRatio, height * CenterRatio });
    const std::array<float, PanelSections + OneCell> x{
        bounds.left, bounds.left + corner, bounds.right - corner, bounds.right };
    const std::array<float, PanelSections + OneCell> y{
        bounds.top, bounds.top + corner, bounds.bottom - corner, bounds.bottom };
    const int sourceLeft = MenuPanelIndex * MenuPanelSourceSize;
    const std::array<int, PanelSections + OneCell> sourceX{
        sourceLeft, sourceLeft + MenuPanelSourceBorder,
        sourceLeft + MenuPanelSourceSize - MenuPanelSourceBorder, sourceLeft + MenuPanelSourceSize };
    const std::array<int, PanelSections + OneCell> sourceY{
        0, MenuPanelSourceBorder, MenuPanelSourceSize - MenuPanelSourceBorder, MenuPanelSourceSize };

    // 角の形を残し、中央と辺だけを引き伸ばす。
    bool drawn = true;
    Render::SpriteBatch sprites(m_render);
    for (int row = 0; row < PanelSections; ++row) {
        for (int column = 0; column < PanelSections; ++column) {
            if (x[column + OneCell] <= x[column] || y[row + OneCell] <= y[row]) continue;
            const SpriteRegion source{ sourceX[column], sourceY[row],
                sourceX[column + OneCell] - sourceX[column], sourceY[row + OneCell] - sourceY[row] };
            const Rect destination{ x[column], y[row], x[column + OneCell], y[row + OneCell] };
            drawn &= m_render.DrawSpriteRegion(*m_menuUi, source, destination);
        }
    }
    return drawn;
}

bool UiRenderer::DrawMenuCharacter(const Rect& bounds, int frame) const {
    return m_knightUi && m_render.DrawFrame(*m_knightUi, frame, bounds);
}

void UiRenderer::DrawScreenText(const wchar_t* text, float centerX, float top, float fontHeight,
    unsigned int color) const {
    const float scale = std::min(static_cast<float>(m_render.Width()) / Stage::WellWidth,
        static_cast<float>(m_render.Height()) / Stage::ViewHeight);
    const float screenLeft = (m_render.Width() - Stage::WellWidth * scale) * CenterRatio;
    const float screenTopOrigin = (m_render.Height() - Stage::ViewHeight * scale) * CenterRatio;
    const int defaultFontSize = GetFontSize();
    if (defaultFontSize <= 0) throw std::runtime_error("GetFontSize failed");

    const int height = std::max(MinHudPixels, static_cast<int>(std::lround(fontHeight * scale)));
    const double textScale = static_cast<double>(height) / defaultFontSize;
    const int length = static_cast<int>(std::wcslen(text));
    const int width = GetDrawExtendStringWidth(textScale, text, length);
    Check(width, "Measure screen text failed");
    const int left = static_cast<int>(std::lround(screenLeft + centerX * scale
        - width * CenterRatio));
    const int screenTop = static_cast<int>(std::lround(screenTopOrigin + top * scale));

    // 標準フォントを拡大して使い、各行を指定位置の中央に揃える。
    Check(DrawExtendString(left, screenTop, textScale, textScale, text, color),
        "Draw screen text failed");
}


}
