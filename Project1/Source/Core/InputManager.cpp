#include "InputManager.h"

#include <Windows.h>
#include <Xinput.h>
#include <DxLib.h>

#pragma comment(lib, "Xinput9_1_0.lib")

namespace downwell {

namespace {

constexpr std::uint64_t GamepadRetryMilliseconds = 1000;
constexpr WORD ExitButtons = XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_START;

}

void InputManager::Update() {
    States states{};

    // 別のウィンドウを操作している間は、ゲームへの入力を止める。
    if (GetForegroundWindow() == GetMainWindowHandle()) {
        states = {
            CheckHitKey(KEY_INPUT_LEFT) != 0 || CheckHitKey(KEY_INPUT_A) != 0,
            CheckHitKey(KEY_INPUT_RIGHT) != 0 || CheckHitKey(KEY_INPUT_D) != 0,
            CheckHitKey(KEY_INPUT_SPACE) != 0,
            CheckHitKey(KEY_INPUT_ESCAPE) != 0
        };

        const auto now = GetTickCount64();
        for (DWORD index = 0; index < m_nextGamepadProbe.size(); ++index) {
            if (now < m_nextGamepadProbe[index]) continue;

            ::XINPUT_STATE gamepad{};
            if (XInputGetState(index, &gamepad) != ERROR_SUCCESS) {
                m_nextGamepadProbe[index] = now + GamepadRetryMilliseconds;
                continue;
            }
            m_nextGamepadProbe[index] = 0;

            const auto buttons = gamepad.Gamepad.wButtons;
            const auto stickX = gamepad.Gamepad.sThumbLX;
            const States padStates = {
                (buttons & XINPUT_GAMEPAD_DPAD_LEFT) != 0 ||
                    stickX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
                (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0 ||
                    stickX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,
                (buttons & XINPUT_GAMEPAD_A) != 0,
                (buttons & ExitButtons) == ExitButtons
            };
            // 入力元をまとめてから押下・解放を判定する。
            for (std::size_t action = 0; action < states.size(); ++action)
                states[action] = states[action] || padStates[action];
        }
    }

    Update(states);
}

void InputManager::Update(const States& states) {
    // 前フレームと比較し、押した瞬間と離した瞬間を区別する。
    m_previous = m_current;
    m_current = states;
}

bool InputManager::Held(Action action) const {
    return m_current[static_cast<std::size_t>(action)];
}

bool InputManager::Pressed(Action action) const {
    const auto index = static_cast<std::size_t>(action);

    return m_current[index] && !m_previous[index];
}

bool InputManager::Released(Action action) const {
    const auto index = static_cast<std::size_t>(action);

    return !m_current[index] && m_previous[index];
}

}
