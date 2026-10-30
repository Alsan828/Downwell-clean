#pragma once

#include "../Graphics/Render.h"
#include "InputManager.h"
#include "../Scenes/SceneManager.h"
#include "../Audio/SoundManager.h"

namespace downwell {

// 起動、メインループ、終了処理を管理する。
class Game final {
public:
    int Run();

private:
    // 逆順に破棄されるため、画像と音声より描画を先に宣言する。
    Render m_renderer;
    SoundManager m_sound;
    InputManager m_input;
    SceneManager m_scenes{ m_input, &m_sound };
};

}
