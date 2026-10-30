#include "Game.h"

#include <DxLib.h>
#include <exception>

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int) {
    try {
        downwell::Game game;
        return game.Run();
    } catch (const std::exception& error) {
        // 起動や実行に失敗した場合は、エラー内容を表示して終了する。
        MessageBoxA(nullptr, error.what(), "Downwell", MB_OK | MB_ICONERROR);
        return 1;
    }
}
