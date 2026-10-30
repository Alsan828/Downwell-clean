#pragma once

#include <algorithm>

namespace downwell {

// ウィンドウの高さから、地面の上面位置を求める。
struct Ground {
    static int Top(int height) {
        return height - std::min(80, height / 4);
    }
};

}
