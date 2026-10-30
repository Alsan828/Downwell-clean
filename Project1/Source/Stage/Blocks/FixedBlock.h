#pragma once

#include "Block.h"

namespace downwell {

// 壊れないブロック。Blockの当たり判定とHitの処理をそのまま使う。
class FixedBlock final : public Block {
public:
    FixedBlock(float left, float top) : Block(left, top) {
    }
};

}
