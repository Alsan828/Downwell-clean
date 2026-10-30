#include "Render.h"
#include "SpriteSheet.h"
#include "../Objects/GameObject.h"
#include "../Stage/Stage.h"
#include "../Objects/Bullets/BulletSystem.h"

#include <DxLib.h>
#include <stdexcept>
#include <cmath>
#include <algorithm>

namespace downwell {

namespace {

constexpr float CenterRatio = 0.5f;
constexpr int FullOpacity = 255;
constexpr int ColorChannelMask = 0xff;
constexpr int GreenChannelShift = 8;
constexpr int RedChannelShift = 16;

Rect EnemyDrawBounds(const Enemy& enemy, const Rect& collision) {
    const auto& sheet = enemy.Sprite();
    const int frame = enemy.Frames().Frame();
    if (!sheet || frame < 0 || frame >= sheet->FrameCount()) return collision;

    // 拡大・回転した絵が画面に残っている間は描画する。
    const Rect image = enemy.AnimationBounds(collision);
    const float halfWidth = (image.right - image.left) * CenterRatio;
    const float halfHeight = (image.bottom - image.top) * CenterRatio;
    const float centerX = (image.left + image.right) * CenterRatio;
    const float centerY = (image.top + image.bottom) * CenterRatio;
    const float angle = enemy.AnimationAngle();
    const float sine = std::abs(std::sin(angle));
    const float cosine = std::abs(std::cos(angle));
    const float extentX = halfWidth * cosine + halfHeight * sine;
    const float extentY = halfWidth * sine + halfHeight * cosine;
    return { centerX - extentX, centerY - extentY, centerX + extentX, centerY + extentY };
}

void Check(int result, const char* operation) {
    // DxLibの失敗を例外にまとめ、呼び出し元で終了処理を行う。
    if (result < 0) throw std::runtime_error(operation);
}

}

Render::SpriteBatch::SpriteBatch(Render& render) {
    // 入れ子では外側の設定を共有する。
    if (render.m_spriteBatch) return;

    int red = 0, green = 0, blue = 0;
    Check(GetDrawBlendMode(&m_saved.blendMode, &m_saved.blendAlpha), "Get frame blend mode failed");
    Check(GetDrawBright(&red, &green, &blue), "Get frame brightness failed");
    m_saved.drawMode = GetDrawMode();
    Check(m_saved.drawMode, "Get frame draw mode failed");
    m_saved.color = (red << RedChannelShift) | (green << GreenChannelShift) | blue;
    m_current = m_saved;
    m_render = &render;
    render.m_spriteBatch = this;
}

Render::SpriteBatch::~SpriteBatch() {
    if (!m_render) return;
    Restore();
    m_render->m_spriteBatch = nullptr;
}

void Render::SpriteBatch::Apply(unsigned int color, int alpha) {
    color &= 0xffffff;
    alpha = std::min(alpha, FullOpacity);
    if (m_current.blendMode != DX_BLENDMODE_ALPHA || m_current.blendAlpha != alpha) {
        Check(SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha), "Set frame blend mode failed");
        m_current.blendMode = DX_BLENDMODE_ALPHA;
        m_current.blendAlpha = alpha;
    }
    if (m_current.color != color) {
        Check(SetDrawBright((color >> RedChannelShift) & ColorChannelMask,
            (color >> GreenChannelShift) & ColorChannelMask, color & ColorChannelMask),
            "Set frame brightness failed");
        m_current.color = color;
    }
    if (m_current.drawMode != DX_DRAWMODE_NEAREST) {
        Check(SetDrawMode(DX_DRAWMODE_NEAREST), "Set sprite draw mode failed");
        m_current.drawMode = DX_DRAWMODE_NEAREST;
    }
}

void Render::SpriteBatch::Restore() noexcept {
    if (!m_render) return;
    if (m_current.color != m_saved.color &&
        SetDrawBright((m_saved.color >> RedChannelShift) & ColorChannelMask,
            (m_saved.color >> GreenChannelShift) & ColorChannelMask,
            m_saved.color & ColorChannelMask) >= 0) m_current.color = m_saved.color;
    if ((m_current.blendMode != m_saved.blendMode || m_current.blendAlpha != m_saved.blendAlpha) &&
        SetDrawBlendMode(m_saved.blendMode, m_saved.blendAlpha) >= 0) {
        m_current.blendMode = m_saved.blendMode;
        m_current.blendAlpha = m_saved.blendAlpha;
    }
    if (m_current.drawMode != m_saved.drawMode && SetDrawMode(m_saved.drawMode) >= 0)
        m_current.drawMode = m_saved.drawMode;
}

Render::~Render() {
    Shutdown();
}

void Render::Initialize() {
    // ウィンドウと描画方式の設定は、DxLib_Initより前に行う。
    Check(SetOutApplicationLogValidFlag(FALSE), "Set library log failed");
    Check(ChangeWindowMode(TRUE), "ChangeWindowMode failed");
    Check(SetGraphMode(m_width, m_height, 32), "SetGraphMode failed");
    Check(SetMainWindowText(L"Downwell - DxLib"), "SetMainWindowText failed");
    Check(SetUseDirect3DVersion(DX_DIRECT3D_11), "Select Direct3D 11 failed");
    // 解像度だけを変え、読み込んだ画像や描画設定は保持する。
    Check(SetChangeScreenModeGraphicsSystemResetFlag(FALSE), "Preserve graphics on resize failed");
    Check(SetWindowSizeChangeEnableFlag(TRUE, TRUE), "Enable window resizing failed");
    Check(SetAlwaysRunFlag(TRUE), "SetAlwaysRunFlag failed");

    Check(SetWaitVSyncFlag(TRUE), "SetWaitVSyncFlag failed");

    if (DxLib_Init() < 0) {
        DxLib_End();
        throw std::runtime_error("DxLib_Init failed");
    }

    m_initialized = true;

    if (GetUseDirect3DVersion() != DX_DIRECT3D_11)
        throw std::runtime_error("DxLib did not initialize Direct3D 11");
}

void Render::Resize(int width, int height) {
    // ウィンドウの拡大率を戻してから描画解像度を変更する。
    Check(SetWindowSizeExtendRate(1.0), "Reset window scale failed");
    Check(SetGraphMode(width, height, 32), "Resize SetGraphMode failed");
    Check(SetWindowSize(width, height), "Resize SetWindowSize failed");

    m_width = width;
    m_height = height;
}

void Render::Draw(const GameObject& object, const Stage& stage, float cameraY, int viewHeight,
    float fadeProgress, const BulletSystem* bullets, float interpolationAlpha, const Player* player,
    float shakeX, float shakeY) {
    m_cameraY = cameraY - shakeY;
    // 移動する物体は主人公と同じ物理時刻で表示する。
    m_interpolationAlpha = std::clamp(interpolationAlpha, 0.0f, 1.0f);
    const int logicalHeight = viewHeight > 0 ? viewHeight : m_height;
    const int logicalWidth = stage.Right() - stage.Left();
    // 縦横の比率を保ち、9列と半分の壁・18行が収まる倍率で表示する。
    m_scale = std::min(static_cast<float>(m_width) / logicalWidth,
        static_cast<float>(m_height) / logicalHeight);
    m_worldLeft = static_cast<float>(stage.Left());
    const float screenLeft = (m_width - logicalWidth * m_scale) * CenterRatio;
    m_screenLeft = screenLeft + shakeX * m_scale;
    m_screenTop = (m_height - logicalHeight * m_scale) * CenterRatio;
    // 裏画面で描画を完成させてから表示し、描画途中のちらつきを防ぐ。
    Check(SetDrawScreen(DX_SCREEN_BACK), "SetDrawScreen failed");
    Check(SetDrawArea(0, 0, m_width, m_height), "Reset draw area failed");
    Check(ClearDrawScreen(), "ClearDrawScreen failed");

    // 井戸の外側を黒く残し、境界の壁は半分だけ見せる。
    Check(SetDrawArea(static_cast<int>(std::lround(screenLeft)), static_cast<int>(std::lround(m_screenTop)),
        static_cast<int>(std::lround(screenLeft + logicalWidth * m_scale)),
        static_cast<int>(std::lround(m_screenTop + logicalHeight * m_scale))),
        "Set well draw area failed");

    {
        SpriteBatch sprites(*this);
        for (const auto* block : stage.Blocks().Nearby(m_cameraY, m_cameraY + logicalHeight)) {
            if (block->Bottom() < m_cameraY || block->Top() > m_cameraY + logicalHeight) continue;
            block->Draw(*this);
        }

        stage.DrawEffects(*this);
        for (const auto& enemy : stage.Enemies()) {
            const Rect collision = enemy->InterpolatedBounds(enemy->PreviousCollision(), m_interpolationAlpha);
            const Rect bounds = EnemyDrawBounds(*enemy, collision);
            if (bounds.bottom < m_cameraY || bounds.top > m_cameraY + logicalHeight) continue;
            enemy->Draw(*this);
        }
        if (bullets) bullets->Draw(*this);
        object.Draw(*this);
        // 命中や砂ぼこりはキャラクターより手前で描く。
        stage.Effects().Draw(*this);
    }

    // 揺らすのは地形と物体だけ。フェードとHUDは元の位置に描く。
    m_screenLeft = screenLeft;
    m_cameraY = cameraY;
    if (fadeProgress > 0.0f) DrawStageFade(fadeProgress, logicalHeight, stage.Left(), stage.Right());

    Check(SetDrawArea(0, 0, m_width, m_height), "Reset draw area failed");
    // HUDは画面に固定し、カメラや井戸の描画範囲に影響されない。
    if (player) m_ui.DrawHud(*player, bullets);
    Check(ScreenFlip(), "ScreenFlip failed");
    m_cameraY = 0.0f;
    m_interpolationAlpha = 1.0f;
    m_scale = 1.0f;
    m_worldLeft = 0.0f;
    m_screenLeft = 0.0f;
    m_screenTop = 0.0f;
}

void Render::BeginScreen() {
    m_cameraY = 0.0f;
    m_interpolationAlpha = 1.0f;
    m_worldLeft = 0.0f;
    m_scale = std::min(static_cast<float>(m_width) / Stage::WellWidth,
        static_cast<float>(m_height) / Stage::ViewHeight);
    m_screenLeft = (m_width - Stage::WellWidth * m_scale) * CenterRatio;
    m_screenTop = (m_height - Stage::ViewHeight * m_scale) * CenterRatio;

    // 井戸のクリップ範囲や透明度を持ち込まず、画面全体を描画する。
    Check(SetDrawScreen(DX_SCREEN_BACK), "SetDrawScreen failed");
    Check(SetDrawArea(0, 0, m_width, m_height), "Reset draw area failed");
    Check(GetDrawBlendMode(&m_screenBlendMode, &m_screenBlendAlpha), "Get blend mode failed");
    Check(SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0), "Set screen blend mode failed");
    Check(ClearDrawScreen(), "ClearDrawScreen failed");
}

void Render::EndScreen() {
    Check(ScreenFlip(), "ScreenFlip failed");
    Check(SetDrawBlendMode(m_screenBlendMode, m_screenBlendAlpha), "Restore blend mode failed");

    m_cameraY = 0.0f;
    m_interpolationAlpha = 1.0f;
    m_scale = 1.0f;
    m_worldLeft = 0.0f;
    m_screenLeft = 0.0f;
    m_screenTop = 0.0f;
}

void Render::DrawStageFade(float progress, int viewHeight, int left, int right) {
    constexpr int StripHeight = 4;
    constexpr float FrontHeight = 0.25f;

    // 境目をぼかした黒い帯を画面の上から下へ送る。
    progress = std::clamp(progress, 0.0f, 1.0f);
    for (int row = 0; row < viewHeight; row += StripHeight) {
        const int bottom = std::min(row + StripHeight, viewHeight);
        const float position = (row + bottom) * 0.5f / viewHeight;
        const float opacity = std::clamp((progress * (1.0f + FrontHeight) - position) / FrontHeight, 0.0f, 1.0f);
        const Rect bounds{ static_cast<float>(left), m_cameraY + row,
            static_cast<float>(right), m_cameraY + bottom };
        DrawFadedRectangle(bounds, 0x000000, static_cast<int>(std::lround(opacity * 255.0f)));
    }
}

void Render::DrawRectangle(const Rect& bounds, unsigned int color, bool filled) {
    if (m_spriteBatch) m_spriteBatch->Restore();
    DrawRectangleImpl(bounds, color, filled);
}

void Render::DrawRectangleImpl(const Rect& bounds, unsigned int color, bool filled) {
    // 小数の座標をピクセルに丸め、矩形の大きさを保って描画する。
    const int left = static_cast<int>(std::lround(m_screenLeft + (bounds.left - m_worldLeft) * m_scale));
    const int top = static_cast<int>(std::lround(m_screenTop + (bounds.top - m_cameraY) * m_scale));
    const int right = static_cast<int>(std::lround(m_screenLeft + (bounds.right - m_worldLeft) * m_scale));
    const int bottom = static_cast<int>(std::lround(m_screenTop + (bounds.bottom - m_cameraY) * m_scale));

    Check(DrawBox(left, top, right, bottom, color, filled ? TRUE : FALSE),
        "DrawBox failed");
}

void Render::DrawFadedRectangle(const Rect& bounds, unsigned int color, int alpha, bool filled) {
    if (alpha <= 0) return;

    if (m_spriteBatch) m_spriteBatch->Restore();
    int previousMode = 0, previousAlpha = 0;
    Check(GetDrawBlendMode(&previousMode, &previousAlpha), "Get blend mode failed");
    Check(SetDrawBlendMode(DX_BLENDMODE_ALPHA, std::min(alpha, 255)), "Set effect blend mode failed");
    DrawRectangleImpl(bounds, color, filled);
    Check(SetDrawBlendMode(previousMode, previousAlpha), "Restore blend mode failed");
}

void Render::DrawRotatedRectangle(const Rect& bounds, float angle, unsigned int color) {
    if (m_spriteBatch) m_spriteBatch->Restore();
    if (angle == 0.0f) {
        DrawRectangle(bounds, color);
        return;
    }

    const float centerX = m_screenLeft + ((bounds.left + bounds.right) * 0.5f - m_worldLeft) * m_scale;
    const float centerY = m_screenTop + ((bounds.top + bounds.bottom) * 0.5f - m_cameraY) * m_scale;
    const float halfWidth = (bounds.right - bounds.left) * 0.5f * m_scale;
    const float halfHeight = (bounds.bottom - bounds.top) * 0.5f * m_scale;
    const float cosine = std::cos(angle);
    const float sine = std::sin(angle);

    struct Point {
        int x;
        int y;
    };

    // 左上から時計回りに四隅を回す。当たり判定の矩形は変更しない。
    const auto rotate = [&](float x, float y) {
        return Point{
            static_cast<int>(std::lround(centerX + x * cosine - y * sine)),
            static_cast<int>(std::lround(centerY + x * sine + y * cosine))
        };
    };
    const Point points[] = {
        rotate(-halfWidth, -halfHeight), rotate(halfWidth, -halfHeight),
        rotate(halfWidth, halfHeight), rotate(-halfWidth, halfHeight)
    };

    Check(DrawQuadrangle(points[0].x, points[0].y, points[1].x, points[1].y,
        points[2].x, points[2].y, points[3].x, points[3].y, color, TRUE),
        "DrawQuadrangle failed");
}

bool Render::DrawFrame(const SpriteSheet& sheet, int frame, const Rect& bounds,
    float angle, bool flipX, unsigned int color, int alpha) {
    if (frame < 0 || frame >= sheet.FrameCount()) return false;

    const int frameWidth = sheet.FrameWidth();
    const int frameHeight = sheet.FrameHeight();
    const SpriteRegion source{ (frame % sheet.Columns()) * frameWidth,
        (frame / sheet.Columns()) * frameHeight, frameWidth, frameHeight };
    return DrawSpriteRegion(sheet, source, bounds, angle, flipX, color, alpha);
}

bool Render::DrawSpriteRegion(const SpriteSheet& sheet, const SpriteRegion& source, const Rect& bounds,
    float angle, bool flipX, unsigned int color, int alpha) {
    const Rect pixels{
        m_screenLeft + (bounds.left - m_worldLeft) * m_scale,
        m_screenTop + (bounds.top - m_cameraY) * m_scale,
        m_screenLeft + (bounds.right - m_worldLeft) * m_scale,
        m_screenTop + (bounds.bottom - m_cameraY) * m_scale
    };
    return DrawSpriteRegionPixels(sheet, source, pixels, angle, flipX, color, alpha);
}

bool Render::DrawSpriteRegionPixels(const SpriteSheet& sheet, const SpriteRegion& source, const Rect& bounds,
    float angle, bool flipX, unsigned int color, int alpha) {
    if (!m_initialized || sheet.FrameCount() <= 0 || source.left < 0 || source.top < 0
        || source.width <= 0 || source.height <= 0 || !std::isfinite(bounds.left)
        || !std::isfinite(bounds.top) || !std::isfinite(bounds.right)
        || !std::isfinite(bounds.bottom) || !std::isfinite(angle)) return false;

    const auto imageWidth = static_cast<long long>(sheet.FrameWidth()) * sheet.Columns();
    const auto imageHeight = static_cast<long long>(sheet.FrameHeight())
        * (sheet.FrameCount() / sheet.Columns());
    if (source.left > imageWidth - source.width || source.top > imageHeight - source.height) return false;

    const float width = bounds.right - bounds.left;
    const float height = bounds.bottom - bounds.top;
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f)
        return false;
    if (alpha <= 0) return true;

    const float centerX = bounds.left + width * CenterRatio;
    const float centerY = bounds.top + height * CenterRatio;
    if (!std::isfinite(centerX) || !std::isfinite(centerY)) return false;

    const double scaleX = static_cast<double>(width) / source.width;
    const double scaleY = static_cast<double>(height) / source.height;

    SpriteBatch sprites(*this);
    m_spriteBatch->Apply(color, alpha);
    return DrawRectRotaGraph3F(centerX, centerY, source.left, source.top, source.width, source.height,
        source.width * CenterRatio, source.height * CenterRatio, scaleX, scaleY, angle,
        sheet.Handle(), TRUE, flipX ? TRUE : FALSE) >= 0;
}

void Render::Shutdown() {
    // 明示的な終了とデストラクタの両方から呼ばれても、解放は一度だけ行う。
    m_ui.Clear();
    if (!m_initialized) return;

    DxLib_End();
    m_initialized = false;
}

}
