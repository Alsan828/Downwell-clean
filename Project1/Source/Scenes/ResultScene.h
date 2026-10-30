#pragma once

#include "Scene.h"

namespace downwell {

// 到達したステージと、タイトルへ戻る操作を表示する画面。
class ResultScene final : public Scene {
public:
    ResultScene(const InputManager& input, int reachedStage);

    void Resize(int width, int height) override;
    SceneTransition Update(float deltaTime) override;
    void Draw(Render& render) const override;

private:
    int m_reachedStage;
};

}
