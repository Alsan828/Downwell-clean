#pragma once

#include "UiRenderer.h"

namespace downwell {

class GameObject;
class Stage;
class BulletSystem;
class Player;
class SpriteSheet;
struct Rect;

struct SpriteRegion {
    int left;
    int top;
    int width;
    int height;
};

// DxLibの初期化、ウィンドウサイズに合わせた描画、終了処理を担当する。
class Render final {
public:
    // 同じ描画設定を続けて使い、スコープの終わりで元へ戻す。
    // スコープ内はRenderの描画関数を使う。
    class SpriteBatch final {
    public:
        explicit SpriteBatch(Render& render);
        ~SpriteBatch();
        SpriteBatch(const SpriteBatch&) = delete;
        SpriteBatch& operator=(const SpriteBatch&) = delete;
        void Restore() noexcept;

    private:
        friend class Render;
        void Apply(unsigned int color, int alpha);

        struct State {
            int blendMode = 0;
            int blendAlpha = 0;
            int drawMode = 0;
            unsigned int color = 0;
        };

        Render* m_render = nullptr;
        State m_saved;
        State m_current;
    };

    ~Render();
    void Initialize();
    void Resize(int width, int height);
    UiRenderer& Ui() { return m_ui; }
    const UiRenderer& Ui() const { return m_ui; }
    bool Initialized() const { return m_initialized; }

    // fadeProgressは0で通常表示、1で上から下まで黒く消える。
    // shakeX/Yはゲーム内の座標単位。地形と物体だけを揺らす。
    void Draw(const GameObject& object, const Stage& stage, float cameraY = 0.0f, int viewHeight = 0,
        float fadeProgress = 0.0f, const BulletSystem* bullets = nullptr, float interpolationAlpha = 1.0f,
        const Player* player = nullptr, float shakeX = 0.0f, float shakeY = 0.0f);
    // 色は0xRRGGBB。filledがfalseの場合は枠だけを描く。
    void DrawRectangle(const Rect& bounds, unsigned int color, bool filled = true);
    // alphaは0～255。ほかの描画へ透明度を持ち越さない。
    void DrawFadedRectangle(const Rect& bounds, unsigned int color, int alpha, bool filled = true);
    // angleはラジアン。中心を軸に描画だけを回転させる。
    void DrawRotatedRectangle(const Rect& bounds, float angle, unsigned int color);
    // 画像がない場合やフレームが範囲外の場合はfalseを返す。
    bool DrawFrame(const SpriteSheet& sheet, int frame, const Rect& bounds,
        float angle = 0.0f, bool flipX = false, unsigned int color = DefaultFrameColor, int alpha = FullFrameAlpha);
    // 元画像は変更せず、指定した範囲だけを取り出して描く。
    bool DrawSpriteRegion(const SpriteSheet& sheet, const SpriteRegion& source, const Rect& bounds,
        float angle = 0.0f, bool flipX = false, unsigned int color = DefaultFrameColor, int alpha = FullFrameAlpha);
    // 画面に固定したUIなどから、ピクセル座標で画像の一部を描く。
    bool DrawSpriteRegionPixels(const SpriteSheet& sheet, const SpriteRegion& source, const Rect& bounds,
        float angle, bool flipX, unsigned int color, int alpha);

    static constexpr unsigned int DefaultFrameColor = 0xffffff;
    static constexpr int FullFrameAlpha = 255;

    // メニューは320×576の座標で描き、ウィンドウの中央に収める。
    void BeginScreen();
    void EndScreen();
    void Shutdown();

    int Width() const { return m_width; }
    int Height() const { return m_height; }
    float InterpolationAlpha() const { return m_interpolationAlpha; }

private:
    void DrawRectangleImpl(const Rect& bounds, unsigned int color, bool filled);
    void DrawStageFade(float progress, int viewHeight, int left, int right);
    UiRenderer m_ui{ *this };
    SpriteBatch* m_spriteBatch = nullptr;

    bool m_initialized = false;
    int m_width = 800;
    int m_height = 18 * 32;
    float m_cameraY = 0.0f;
    float m_interpolationAlpha = 1.0f;
    float m_scale = 1.0f;
    float m_worldLeft = 0.0f;
    float m_screenLeft = 0.0f;
    float m_screenTop = 0.0f;
    int m_screenBlendMode = 0;
    int m_screenBlendAlpha = 0;
};

}
