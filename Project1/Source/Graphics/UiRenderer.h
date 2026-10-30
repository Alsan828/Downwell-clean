#pragma once

#include <filesystem>
#include <memory>

namespace downwell {

class Render;
class SpriteSheet;
class Player;
class BulletSystem;
struct Rect;

// HUDとメニューの配置・文字・画像を管理し、図形の描画はRenderへ渡す。
class UiRenderer final {
public:
    explicit UiRenderer(Render& render);
    UiRenderer(const UiRenderer&) = delete;
    UiRenderer& operator=(const UiRenderer&) = delete;

    bool LoadSprites(const std::filesystem::path& dataRoot);
    void Clear();
    void DrawHud(const Player& player, const BulletSystem* bullets) const;
    bool DrawMenuPanel(const Rect& bounds) const;
    bool DrawMenuCharacter(const Rect& bounds, int frame = 0) const;
    void DrawScreenText(const wchar_t* text, float centerX, float top, float fontHeight,
        unsigned int color) const;

private:
    void DrawSkinnedHud(const Player& player, const BulletSystem* bullets) const;

    // Renderが所有し、DxLibの終了前にUI画像を解放する。
    Render& m_render;
    std::shared_ptr<const SpriteSheet> m_healthUi;
    std::shared_ptr<const SpriteSheet> m_menuUi;
    std::shared_ptr<const SpriteSheet> m_knightUi;
};

}
