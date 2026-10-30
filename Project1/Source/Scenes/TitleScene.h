#pragma once

#include "Scene.h"

namespace downwell {

// 開始操作と、操作説明を表示する画面。
class TitleScene final : public Scene {
public:
    explicit TitleScene(const InputManager& input);

    void Resize(int width, int height) override;
    SceneTransition Update(float deltaTime) override;
    void Draw(Render& render) const override;

private:
    float m_blinkTime = 0.0f;
};

}
