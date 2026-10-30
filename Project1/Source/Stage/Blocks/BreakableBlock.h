#pragma once

#include "Block.h"

namespace downwell {

// 下から頭が当たると壊れるブロック。枠だけで描画する。
class BreakableBlock final : public Block {
public:
    BreakableBlock(float left, float top) : Block(left, top) {
    }

    bool IsBreakable() const override { return true; }

    void Hit() override {
        Destroy();
    }

    void Destroy() {
        // すぐに判定と描画の対象から外し、Stageが後で一覧から取り除く。
        m_destroyed = true;
    }
};

}
