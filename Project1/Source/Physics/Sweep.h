#pragma once

#include "Collider.h"

#include <algorithm>

namespace downwell {

inline bool SweepAxis(float low, float high, float otherLow, float otherHigh, float movement,
    float& entry, float& exit) {
    if (movement == 0.0f) return high > otherLow && low < otherHigh;

    const float first = (otherLow - high) / movement;
    const float last = (otherHigh - low) / movement;
    entry = std::max(entry, std::min(first, last));
    exit = std::min(exit, std::max(first, last));
    return entry <= exit;
}

// 移動前後の相対移動から、この刻みで初めて接触する時刻を求める。
inline float SweepContactTime(const Rect& body, const Rect& before, const Rect& after,
    float moveX, float moveY) {
    float entry = 0.0f;
    float exit = 1.0f;
    if (!SweepAxis(body.left, body.right, before.left, before.right,
        moveX - (after.left - before.left), entry, exit) ||
        !SweepAxis(body.top, body.bottom, before.top, before.bottom,
            moveY - (after.top - before.top), entry, exit)) return -1.0f;

    // 辺から離れるだけの接触は含めず、実際の重なりは引き続き判定する。
    return exit > 0.0f ? entry : -1.0f;
}

}
