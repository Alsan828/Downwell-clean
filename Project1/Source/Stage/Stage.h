#pragma once

#include "Blocks/Block.h"
#include "StageData.h"
#include "../Objects/Enemies/Enemy.h"
#include "../Graphics/SpriteEffects.h"

#include <random>

namespace downwell {

// 地面と壁は固定ブロック、足場は二種類のブロックで配置する。
class Stage final {
public:
    void Generate(int width);
    void Generate(int width, std::mt19937& random);
    void Generate(int width, const StageData& data, std::mt19937& random);
    void Update(float deltaTime);
    void RemoveDestroyedBlocks();
    void DrawEffects(Render& render) const;
    // 消滅演出に使う画像と、一度だけ再生するコマを指定する。
    void SetDisappearAnimation(std::shared_ptr<const SpriteSheet> sheet, const AnimationClip& clip);
    void SetBlockSprites(std::shared_ptr<const SpriteSheet> sheet,
        int fixedFrame, int breakableFrame, int wallFrame);
    void SetEnemySprites(const EnemySprite& ground, const EnemySprite& floating, const EnemySprite& turtle);

    BlockList& Blocks() { return m_blocks; }
    const BlockList& Blocks() const { return m_blocks; }
    EnemyList& Enemies() { return m_enemies; }
    const EnemyList& Enemies() const { return m_enemies; }
    SpriteEffects& Effects() { return m_spriteEffects; }
    const SpriteEffects& Effects() const { return m_spriteEffects; }

    int Left() const { return m_left; }
    int Right() const { return m_right; }
    int Height() const { return m_height; }
    const std::vector<std::string>& SelectedChunks() const { return m_selectedChunks; }
    static constexpr int WellWidth = (StageData::Columns + 1) * Block::TileSize;
    static constexpr int ViewHeight = 18 * Block::TileSize;

private:
    void GenerateEnemies(int top, int bottom, std::mt19937& random);
    void ApplyBlockSprites();
    void ApplyEnemySprites();
    void AddDisappearEffect(const Rect& bounds, unsigned int color, bool filled = true,
        const GameObject* source = nullptr);

    struct DisappearEffect {
        Rect bounds;
        unsigned int color;
        bool filled;
        float time = 0.0f;
        std::shared_ptr<const SpriteSheet> spriteSheet;
        FrameAnimation animation;
        bool playFrames = false;
        float angle = 0.0f;
        bool flipX = false;
    };

    BlockList m_blocks;
    EnemyList m_enemies;
    // 消えたブロックと敵の見た目だけを保存し、同じ演出で描く。
    std::vector<DisappearEffect> m_disappearEffects;
    SpriteEffects m_spriteEffects;
    std::shared_ptr<const SpriteSheet> m_disappearSprite;
    FrameAnimation m_disappearAnimation;
    std::shared_ptr<const SpriteSheet> m_blockSprites;
    int m_fixedFrame = 0;
    int m_breakableFrame = 0;
    int m_wallFrame = 0;
    EnemySprite m_groundSprite;
    EnemySprite m_floatingSprite;
    EnemySprite m_turtleSprite;
    StageData m_data;
    std::vector<std::string> m_selectedChunks;
    int m_left = 0;
    int m_right = 0;
    int m_height = 0;
    static constexpr float DisappearEffectDuration = 0.25f;
    static constexpr float DisappearEffectMaxScale = 2.0f;
};

}
