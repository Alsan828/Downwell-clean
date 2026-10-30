#include "SpriteSheet.h"

#include <DxLib.h>
#include <limits>

namespace downwell {

namespace {

constexpr int InvalidHandle = -1;

}

struct SpriteSheet::Graph {
    ~Graph() {
        if (handle != InvalidHandle) DeleteGraph(handle);
    }

    int handle = InvalidHandle;
    int frameWidth = 0;
    int frameHeight = 0;
    int columns = 0;
    int frameCount = 0;
};

SpriteSheet::SpriteSheet() = default;

SpriteSheet::~SpriteSheet() = default;

bool SpriteSheet::Load(const std::filesystem::path& file, int frameWidth, int frameHeight) {
    if (file.empty() || frameWidth <= 0 || frameHeight <= 0) return false;

    auto next = std::make_unique<Graph>();
    next->handle = LoadGraph(file.c_str());
    if (next->handle < 0) return false;

    // DxLibが先に終了した場合も、残った共有画像を安全に破棄できる。
    if (SetDeleteHandleFlag(next->handle, &next->handle) < 0) return false;

    int width = 0;
    int height = 0;
    if (GetGraphSize(next->handle, &width, &height) < 0 || width <= 0 || height <= 0 ||
        width % frameWidth != 0 || height % frameHeight != 0) return false;

    const int columns = width / frameWidth;
    const int rows = height / frameHeight;
    const auto count = static_cast<long long>(columns) * rows;
    if (count <= 0 || count > std::numeric_limits<int>::max()) return false;

    next->frameWidth = frameWidth;
    next->frameHeight = frameHeight;
    next->columns = columns;
    next->frameCount = static_cast<int>(count);
    m_graph.swap(next);
    return true;
}

void SpriteSheet::Clear() {
    m_graph.reset();
}

int SpriteSheet::Handle() const {
    return m_graph ? m_graph->handle : InvalidHandle;
}

int SpriteSheet::FrameCount() const {
    return Handle() >= 0 ? m_graph->frameCount : 0;
}

int SpriteSheet::FrameWidth() const {
    return Handle() >= 0 ? m_graph->frameWidth : 0;
}

int SpriteSheet::FrameHeight() const {
    return Handle() >= 0 ? m_graph->frameHeight : 0;
}

int SpriteSheet::Columns() const {
    return Handle() >= 0 ? m_graph->columns : 0;
}

}
