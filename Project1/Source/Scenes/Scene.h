#pragma once

namespace downwell {

class InputManager;
class Render;

enum class SceneType { None, Title, Play, Result };

// 更新が終わってから画面を切り替えるための要求。
struct SceneTransition {
    SceneType next = SceneType::None;
    int reachedStage = 0;
};

// 各画面が持つ、更新・描画・サイズ変更の共通インターフェース。
class Scene {
public:
    virtual ~Scene() = default;

    virtual void Resize(int width, int height) = 0;
    virtual SceneTransition Update(float deltaTime) = 0;
    virtual void Draw(Render& render) const = 0;

protected:
    explicit Scene(const InputManager& input) : m_input(input) {
    }

    // Gameが持つ入力を参照する。画面側では入力状態を変更しない。
    const InputManager& m_input;
};

}
