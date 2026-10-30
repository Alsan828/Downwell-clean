#include "SceneManager.h"
#include "TitleScene.h"
#include "PlayScene.h"
#include "ResultScene.h"

#include <utility>

namespace downwell {

SceneManager::SceneManager(const InputManager& input, SoundManager* sound)
    : m_input(input), m_sound(sound) {
}

void SceneManager::ChangeScene(std::unique_ptr<Scene> scene) {
    // 新しい画面の所有権を受け取り、前の画面はここで解放する。
    m_currentScene = std::move(scene);
    if (m_currentScene && m_width > 0 && m_height > 0) {
        m_currentScene->Resize(m_width, m_height);
    }
}

void SceneManager::Resize(int width, int height) {
    m_width = width;
    m_height = height;
    if (m_currentScene) m_currentScene->Resize(width, height);
}

void SceneManager::Update(float deltaTime) {
    if (!m_currentScene) return;

    // 古い画面の更新が戻ってから解放し、新しい画面は次のフレームで更新する。
    const SceneTransition transition = m_currentScene->Update(deltaTime);

    switch (transition.next) {
    case SceneType::Title:
        ChangeScene(std::make_unique<TitleScene>(m_input));
        break;
    case SceneType::Play:
        ChangeScene(std::make_unique<PlayScene>(m_input, m_sound));
        break;
    case SceneType::Result:
        ChangeScene(std::make_unique<ResultScene>(m_input, transition.reachedStage));
        break;
    case SceneType::None:
        break;
    }
}

void SceneManager::Draw(Render& render) const {
    if (m_currentScene) m_currentScene->Draw(render);
}

}
