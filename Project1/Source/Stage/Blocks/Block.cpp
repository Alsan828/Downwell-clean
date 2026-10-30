#include "Block.h"
#include "../../Graphics/Render.h"

namespace downwell {

void Block::Draw(Render& render) const {
    if (Destroyed()) return;
    if (DrawAnimation(render, GetCollision())) return;

    // 固定ブロックは塗りつぶし、破壊できるブロックは枠だけで区別する。
    if (!IsBreakable()) render.DrawRectangle(GetCollision(), 0xa0a0a0);
    render.DrawRectangle(GetCollision(), 0xdcdcdc, false);
}

}
