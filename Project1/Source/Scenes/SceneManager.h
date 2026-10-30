#pragma once

#include "Scene.h"

#include <memory>

namespace downwell {

class SoundManager;

// 現在の画面を所有し、更新と描画をその画面へ渡す。
class SceneManager final {
public:
    explicit SceneManager(const InputManager& input, SoundManager* sound = nullptr);

    void ChangeScene(std::unique_ptr<Scene> scene);
    void Resize(int width, int height);
    void Update(float deltaTime);
    void Draw(Render& render) const;

private:
    const InputManager& m_input;
    SoundManager* m_sound;
    std::unique_ptr<Scene> m_currentScene;
    int m_width = 0;
    int m_height = 0;
};

}
