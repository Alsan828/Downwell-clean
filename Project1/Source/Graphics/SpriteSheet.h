#pragma once

#include <filesystem>
#include <memory>

namespace downwell {

class Render;

// 等間隔に並んだ画像を共有し、各フレームを左上から順に扱う。
class SpriteSheet final {
public:
    SpriteSheet();
    ~SpriteSheet();
    SpriteSheet(const SpriteSheet&) = delete;
    SpriteSheet& operator=(const SpriteSheet&) = delete;

    // 失敗した場合は、現在の画像をそのまま残す。
    bool Load(const std::filesystem::path& file, int frameWidth, int frameHeight);
    void Clear();
    int FrameCount() const;
    int FrameWidth() const;
    int FrameHeight() const;

private:
    friend class Render;
    struct Graph;

    int Handle() const;
    int Columns() const;

    std::unique_ptr<Graph> m_graph;
};

}
