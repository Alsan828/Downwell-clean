#include "Stage.h"
#include "Blocks/FixedBlock.h"
#include "Blocks/BreakableBlock.h"
#include "../Graphics/Render.h"
#include "../Graphics/SpriteSheet.h"
#include "../Objects/Enemies/GroundEnemy.h"
#include "../Objects/Enemies/TurtleEnemy.h"
#include "../Objects/Enemies/FloatingEnemy.h"

#include <algorithm>
#include <numeric>
#include <cmath>
#include <utility>

namespace downwell {

namespace {

constexpr int SingleFrameCount = 1;

std::mt19937 CreateRandomEngine() {
    std::random_device source;
    std::seed_seq seed{ source(), source(), source(), source() };
    return std::mt19937(seed);
}

}

void Stage::Generate(int width) {
    // 起動時に種を作り、次のプレイでも同じ乱数器を進めて使う。
    static std::mt19937 random = CreateRandomEngine();
    static std::vector<std::string> previousChunks;

    StageData data;
    data.Load(StageData::DefaultPath());

    // 前のプレイと全く同じ並びになった場合だけ、もう一度選ぶ。
    do {
        Generate(width, data, random);
    } while (m_selectedChunks == previousChunks);

    previousChunks = m_selectedChunks;
}

void Stage::Generate(int width, std::mt19937& random) {
    StageData data;
    data.Load(StageData::DefaultPath());
    Generate(width, data, random);
}

void Stage::Generate(int width, const StageData& data, std::mt19937& random) {
    m_data = data;
    m_blocks.clear();
    m_enemies.clear();
    m_disappearEffects.clear();
    m_spriteEffects.Clear();
    m_selectedChunks.clear();
    m_left = (width - WellWidth) / 2;
    m_right = m_left + WellWidth;

    // 候補を並べ替え、先頭の5個を使う。同じ区画は一度しか選ばない。
    std::vector<std::size_t> order(m_data.Middle().size());
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), random);
    std::vector<const Chunk*> chunks{ &m_data.Start() };
    for (int index = 0; index < StageData::MiddleCount; ++index) {
        chunks.push_back(&m_data.Middle().at(order.at(index)));
    }
    chunks.push_back(&m_data.End());

    const int size = Block::TileSize;
    std::vector<std::pair<int, int>> middleRanges;
    int rowOffset = 0;
    for (const Chunk* chunk : chunks) {
        m_selectedChunks.push_back(chunk->id);
        for (int row = 0; row < chunk->Rows(); ++row) {
            for (int column = 0; column < StageData::Columns; ++column) {
                const int cell = chunk->cells[row][column];
                const float left = m_left + size * 0.5f + column * size;
                const float top = static_cast<float>((rowOffset + row) * size);
                if (cell == 1) m_blocks.push_back(std::make_unique<FixedBlock>(left, top));
                else if (cell == 2) m_blocks.push_back(std::make_unique<BreakableBlock>(left, top));
            }
        }
        if (chunk != &m_data.Start() && chunk != &m_data.End()) {
            middleRanges.emplace_back(rowOffset * size, (rowOffset + chunk->Rows()) * size);
        }
        rowOffset += chunk->Rows();
    }
    m_height = rowOffset * size;

    // 全区画を通して同じ壁を置く。境界の内側には半分だけ見える。
    for (int row = 0; row < rowOffset; ++row) {
        const float top = static_cast<float>(row * size);
        m_blocks.push_back(std::make_unique<FixedBlock>(m_left - size * 0.5f, top));
        m_blocks.push_back(std::make_unique<FixedBlock>(m_right - size * 0.5f, top));
    }

    for (const auto& range : middleRanges) GenerateEnemies(range.first, range.second, random);
    ApplyBlockSprites();
    ApplyEnemySprites();
}

void Stage::SetEnemySprites(const EnemySprite& ground, const EnemySprite& floating, const EnemySprite& turtle) {
    // 読み込み失敗時は既存の画像を残し、未設定の種類は矩形で描く。
    if (ground.Valid()) m_groundSprite = ground;
    if (floating.Valid()) m_floatingSprite = floating;
    if (turtle.Valid()) m_turtleSprite = turtle;
    ApplyEnemySprites();
}

void Stage::ApplyEnemySprites() {
    for (const auto& enemy : m_enemies) {
        const EnemySprite* sprite = nullptr;
        if (dynamic_cast<const GroundEnemy*>(enemy.get())) sprite = &m_groundSprite;
        else if (dynamic_cast<const FloatingEnemy*>(enemy.get())) sprite = &m_floatingSprite;
        else if (dynamic_cast<const TurtleEnemy*>(enemy.get())) sprite = &m_turtleSprite;
        if (sprite) enemy->SetAnimationSprite(*sprite);
    }
}

void Stage::SetBlockSprites(std::shared_ptr<const SpriteSheet> sheet,
    int fixedFrame, int breakableFrame, int wallFrame) {
    if (sheet) {
        const int count = sheet->FrameCount();
        if (fixedFrame < 0 || fixedFrame >= count || breakableFrame < 0 || breakableFrame >= count ||
            wallFrame < 0 || wallFrame >= count) return;
    }
    m_blockSprites = std::move(sheet);
    m_fixedFrame = fixedFrame;
    m_breakableFrame = breakableFrame;
    m_wallFrame = wallFrame;
    ApplyBlockSprites();
}

void Stage::ApplyBlockSprites() {
    for (const auto& block : m_blocks) {
        block->SetSpriteSheet(m_blockSprites);
        if (!m_blockSprites) continue;

        const bool wall = block->Left() < m_left || block->Right() > m_right;
        const int frame = wall ? m_wallFrame : (block->IsBreakable() ? m_breakableFrame : m_fixedFrame);
        block->Frames().SetClip({ frame, SingleFrameCount, AnimationClip::DefaultFrameDuration, true });
    }
}

void Stage::GenerateEnemies(int top, int bottom, std::mt19937& random) {
    struct Position { float x; float y; };
    std::vector<Position> groundPositions;
    std::vector<Position> floatingPositions;
    const int tile = Block::TileSize;
    const bool turtle = std::bernoulli_distribution(0.5)(random);
    const int groundWidth = turtle ? TurtleEnemy::BodyWidth : Enemy::Size;
    const int groundHeight = turtle ? TurtleEnemy::BodyHeight : Enemy::Size;

    const auto clear = [&](float x, float y, float travel, int width = Enemy::Size, int height = Enemy::Size) {
        const Rect area{ x - width * 0.5f, y - height * 0.5f - travel,
            x + width * 0.5f, y + height * 0.5f + travel };
        for (const auto* block : m_blocks.Nearby(area.top, area.bottom)) {
            const Rect& bounds = block->GetCollision();
            if (area.right > bounds.left && area.left < bounds.right &&
                area.bottom > bounds.top && area.top < bounds.bottom) return false;
        }
        // 別の敵と重ならないよう、一体分以上の間隔を空ける。
        for (const auto& enemy : m_enemies) {
            if (std::hypot(x - enemy->X(), y - enemy->Y()) < tile * 2.0f) return false;
        }
        return true;
    };

    // 足場の上を候補にする。開始区画には敵を置かない。
    for (const auto& block : m_blocks) {
        if (block->X() <= m_left + tile * 0.5f ||
            block->X() >= m_right - tile * 0.5f) continue;
        if (block->Top() < top + tile * 2 || block->Top() >= bottom) continue;
        const float y = block->Top() - groundHeight * 0.5f;
        if (clear(block->X(), y, 0.0f, groundWidth, groundHeight)) groundPositions.push_back({ block->X(), y });
    }
    std::shuffle(groundPositions.begin(), groundPositions.end(), random);
    if (!groundPositions.empty()) {
        const Position position = groundPositions.front();
        if (turtle) m_enemies.push_back(std::make_unique<TurtleEnemy>(position.x, position.y));
        else m_enemies.push_back(std::make_unique<GroundEnemy>(position.x, position.y));
    }

    // 浮遊敵は空いているマスに置き、出現後に主人公を追う。
    for (int row = top + tile * 2; row + tile * 2 < bottom; row += tile) {
        for (int column = 0; column < StageData::Columns; ++column) {
            const float x = static_cast<float>(m_left + (column + 1) * tile);
            const float y = row + tile * 0.5f;
            if (clear(x, y, 0.0f)) floatingPositions.push_back({ x, y });
        }
    }
    std::shuffle(floatingPositions.begin(), floatingPositions.end(), random);
    if (!floatingPositions.empty()) {
        const Position position = floatingPositions.front();
        m_enemies.push_back(std::make_unique<FloatingEnemy>(position.x, position.y));
    }
}

void Stage::Update(float deltaTime) {
    m_spriteEffects.Update(deltaTime);
    for (const auto& block : m_blocks) {
        if (!block->Destroyed()) block->UpdateFrames(deltaTime);
    }
    // 既存の演出を進めてから、新しく消えたオブジェクトの演出を追加する。
    for (auto& effect : m_disappearEffects) {
        effect.time += std::max(deltaTime, 0.0f);
        effect.animation.Update(deltaTime);
    }
    m_disappearEffects.erase(std::remove_if(m_disappearEffects.begin(), m_disappearEffects.end(), [](const auto& effect) {
        return effect.playFrames ? effect.animation.Finished() : effect.time >= DisappearEffectDuration;
    }), m_disappearEffects.end());

    RemoveDestroyedBlocks();
    for (const auto& enemy : m_enemies) {
        if (!enemy->Active()) {
            if (!m_spriteEffects.Add({ SpriteEffectType::EnemyDisappear, enemy->X(), enemy->Y() })) {
                AddDisappearEffect(enemy->GetCollision(), Enemy::Color, enemy->Filled(), enemy.get());
            }
        }
    }
    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const auto& enemy) {
        return !enemy->Active();
    }), m_enemies.end());
}

void Stage::RemoveDestroyedBlocks() {
    // 衝突判定の途中では消去せず、移動処理が終わってから取り除く。
    for (const auto& block : m_blocks) {
        if (block->Destroyed() && block->IsBreakable()) {
            AddDisappearEffect(block->GetCollision(), 0xffffff, false, block.get());
            m_spriteEffects.Add({ SpriteEffectType::Dust, block->X(), block->Y() });
        }
    }
    m_blocks.erase(std::remove_if(m_blocks.begin(), m_blocks.end(), [](const auto& block) {
        return block->Destroyed();
    }), m_blocks.end());
}

void Stage::AddDisappearEffect(const Rect& bounds, unsigned int color, bool filled, const GameObject* source) {
    DisappearEffect effect{ bounds, color, filled };
    effect.spriteSheet = m_disappearSprite;
    effect.animation = m_disappearAnimation;
    effect.animation.Restart();
    effect.playFrames = static_cast<bool>(m_disappearSprite);
    // 専用アニメーションがなければ、消える直前の画像を拡大して薄くする。
    if (!effect.playFrames && source && source->Sprite()) {
        effect.spriteSheet = source->Sprite();
        effect.animation.SetClip({ source->Frames().Frame(), SingleFrameCount, AnimationClip::DefaultFrameDuration, true });
        effect.color = GameObject::FrameTint;
        if (const auto* enemy = dynamic_cast<const Enemy*>(source)) {
            effect.bounds = enemy->AnimationBounds(bounds);
            effect.angle = enemy->AnimationAngle();
            effect.flipX = enemy->AnimationFlipX();
        }
    }
    m_disappearEffects.push_back(std::move(effect));
}

void Stage::SetDisappearAnimation(std::shared_ptr<const SpriteSheet> sheet, const AnimationClip& clip) {
    m_disappearSprite = std::move(sheet);
    AnimationClip effectClip = clip;
    if (m_disappearSprite && m_disappearSprite->FrameCount() > 0) {
        const int frames = m_disappearSprite->FrameCount();
        effectClip.firstFrame = std::clamp(effectClip.firstFrame, 0, frames - 1);
        effectClip.frameCount = std::clamp(effectClip.frameCount, 1, frames - effectClip.firstFrame);
    } else {
        m_disappearSprite.reset();
    }
    effectClip.loop = false;
    m_disappearAnimation.SetClip(effectClip);
}

void Stage::DrawEffects(Render& render) const {
    for (const auto& effect : m_disappearEffects) {
        if (effect.playFrames && effect.spriteSheet &&
            render.DrawFrame(*effect.spriteSheet, effect.animation.Frame(), effect.bounds)) continue;
        const float remaining = std::clamp(1.0f - effect.time / DisappearEffectDuration, 0.0f, 1.0f);
        // 最初に素早く広がり、元の中心位置で薄くなって消える。
        const float scale = 1.0f + (DisappearEffectMaxScale - 1.0f) * (1.0f - remaining * remaining);
        const float x = (effect.bounds.left + effect.bounds.right) * 0.5f;
        const float y = (effect.bounds.top + effect.bounds.bottom) * 0.5f;
        const float halfWidth = (effect.bounds.right - effect.bounds.left) * scale * 0.5f;
        const float halfHeight = (effect.bounds.bottom - effect.bounds.top) * scale * 0.5f;
        const Rect bounds{ x - halfWidth, y - halfHeight, x + halfWidth, y + halfHeight };
        const int alpha = static_cast<int>(std::lround(remaining * GameObject::FrameOpacity));
        if (effect.spriteSheet &&
            render.DrawFrame(*effect.spriteSheet, effect.animation.Frame(), bounds,
                effect.angle, effect.flipX, effect.color, alpha)) continue;
        render.DrawFadedRectangle(bounds, effect.color, alpha, effect.filled);
    }
}

}
