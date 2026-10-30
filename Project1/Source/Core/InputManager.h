#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

// キーそのものではなく、ゲームで使う操作を表す。
enum class Action : std::size_t {
    Left,
    Right,
    JumpShoot,
    Exit,
    Count
};

// 現在と前フレームの入力を保存し、押下・解放を判定する。
class InputManager final {
public:
    using States = std::array<bool, static_cast<std::size_t>(Action::Count)>;

    void Update();
    void Update(const States& states);

    // Heldは押している間、PressedとReleasedは切り替わったフレームだけtrue。
    bool Held(Action action) const;
    bool Pressed(Action action) const;
    bool Released(Action action) const;

private:
    States m_current{};
    States m_previous{};
    // XInputの4スロット。未接続のスロットは間隔を空けて再確認する。
    std::array<std::uint64_t, 4> m_nextGamepadProbe{};
};

}
