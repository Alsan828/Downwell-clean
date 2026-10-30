#pragma once

namespace downwell::physics {

// 主人公と敵は同じ物理刻みで動かす。時間の単位は秒。
constexpr double UpdatesPerSecond = 120.0;
constexpr double Step = 1.0 / UpdatesPerSecond;
constexpr double AccumulatorTolerance = 1.0e-9;

}
