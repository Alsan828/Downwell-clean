#pragma once

#include <algorithm>
#include <cmath>

namespace downwell {

// 下向きに加速し、落下速度が上限を超えないようにする。
class Gravity final {
public:
    Gravity(float acceleration, float maxFallSpeed)
        : m_acceleration(acceleration), m_maxFallSpeed(maxFallSpeed) {
    }

    void Apply(float& verticalSpeed, float deltaTime, bool softenApex = false) const {
        float scale = 1.0f;
        if (softenApex) {
            // 頂点に近いほど重力を弱め、速度を止めずに滑らかに戻す。
            const float speedRatio = std::clamp(std::abs(verticalSpeed) / ApexSpeedRange, 0.0f, 1.0f);
            const float blend = speedRatio * speedRatio * (3.0f - 2.0f * speedRatio);
            scale = ApexGravityRatio + (1.0f - ApexGravityRatio) * blend;
        }

        // deltaTimeは秒単位。速度に加速度×時間を加える。
        verticalSpeed = std::min(verticalSpeed + m_acceleration * scale * deltaTime, m_maxFallSpeed);
    }

    static constexpr float ApexSpeedRange = 100.0f;
    static constexpr float ApexGravityRatio = 0.65f;

private:
    float m_acceleration;
    float m_maxFallSpeed;
};

}
