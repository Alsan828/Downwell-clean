#pragma once

namespace downwell {

// 矩形の左・上・右・下の座標。
struct Rect {
    float left;
    float top;
    float right;
    float bottom;
};

// 中心座標と大きさから、共通の矩形判定を作る。
class Collider final {
public:
    void UpdateBounds(float x, float y, int width, int height) {
        m_bounds = { x - width * 0.5f, y - height * 0.5f,
            x + width * 0.5f, y + height * 0.5f };
    }

    bool Overlaps(const Collider& other) const {
        // 辺が触れているだけの場合は、重なっているとは判定しない。
        return m_bounds.right > other.m_bounds.left && m_bounds.left < other.m_bounds.right &&
            m_bounds.bottom > other.m_bounds.top && m_bounds.top < other.m_bounds.bottom;
    }

    const Rect& Bounds() const { return m_bounds; }

private:
    Rect m_bounds{};
};

}
