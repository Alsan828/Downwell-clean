#pragma once

#include "../../Objects/GameObject.h"

#include <memory>
#include <vector>
#include <algorithm>

namespace downwell {

// 地形ブロックの共通サイズと当たり判定の範囲。
class Block : public GameObject {
public:
    static constexpr int TileSize = 32;

    float Left() const { return GetCollision().left; }
    float Right() const { return GetCollision().right; }
    float Top() const { return GetCollision().top; }
    float Bottom() const { return GetCollision().bottom; }

    void Draw(Render& render) const override;

    virtual bool IsBreakable() const { return false; }
    // 当たったときの処理。固定ブロックは何もせず、破壊可能ブロックで変更する。
    virtual void Hit() {}
    bool Destroyed() const { return m_destroyed; }

protected:
    Block(float left, float top) : GameObject(TileSize, TileSize) {
        SetPosition(left + TileSize * 0.5f, top + TileSize * 0.5f);
    }

    bool m_destroyed = false;
};

// ブロックを所有し、高さ順の索引で近くの地形だけを取り出す。
class BlockList final {
public:
    using Storage = std::vector<std::unique_ptr<Block>>;
    using iterator = Storage::iterator;
    using const_iterator = Storage::const_iterator;

    struct Range {
        std::vector<Block*>::const_iterator first;
        std::vector<Block*>::const_iterator last;
        auto begin() const { return first; }
        auto end() const { return last; }
    };

    void push_back(std::unique_ptr<Block> block) {
        m_blocks.push_back(std::move(block));
        m_indexDirty = true;
    }

    void clear() {
        m_blocks.clear();
        m_index.clear();
        m_indexDirty = false;
    }

    iterator erase(iterator first, iterator last) {
        if (first == last) return first;
        m_indexDirty = true;
        return m_blocks.erase(first, last);
    }

    iterator erase(iterator position) {
        m_indexDirty = true;
        return m_blocks.erase(position);
    }

    iterator begin() { return m_blocks.begin(); }
    iterator end() { return m_blocks.end(); }
    const_iterator begin() const { return m_blocks.begin(); }
    const_iterator end() const { return m_blocks.end(); }
    bool empty() const { return m_blocks.empty(); }
    std::size_t size() const { return m_blocks.size(); }
    auto& operator[](std::size_t index) { m_indexDirty = true; return m_blocks[index]; }
    const auto& operator[](std::size_t index) const { return m_blocks[index]; }
    auto& at(std::size_t index) { m_indexDirty = true; return m_blocks.at(index); }
    const auto& at(std::size_t index) const { return m_blocks.at(index); }
    auto& front() { m_indexDirty = true; return m_blocks.front(); }
    const auto& front() const { return m_blocks.front(); }
    auto& back() { m_indexDirty = true; return m_blocks.back(); }
    const auto& back() const { return m_blocks.back(); }

    // 移動する地形を追加する場合は、位置を変えた後に索引も更新する。
    void InvalidateIndex() { m_indexDirty = true; }

    Range Nearby(float top, float bottom) const {
        if (m_indexDirty) {
            m_index.clear();
            m_index.reserve(m_blocks.size());
            for (const auto& block : m_blocks) m_index.push_back(block.get());
            std::stable_sort(m_index.begin(), m_index.end(), [](const Block* left, const Block* right) {
                return left->Top() < right->Top();
            });
            m_indexDirty = false;
        }
        const float firstTop = std::min(top, bottom) - Block::TileSize;
        const float lastTop = std::max(top, bottom);
        const auto first = std::lower_bound(m_index.begin(), m_index.end(), firstTop,
            [](const Block* block, float value) { return block->Top() < value; });
        const auto last = std::upper_bound(first, m_index.end(), lastTop,
            [](float value, const Block* block) { return value < block->Top(); });
        return { first, last };
    }

private:
    Storage m_blocks;
    mutable std::vector<Block*> m_index;
    mutable bool m_indexDirty = true;
};

}
