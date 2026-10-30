#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace downwell {

struct Chunk {
    std::string id;
    std::string name;
    std::vector<std::vector<int>> cells;
    int Rows() const { return static_cast<int>(cells.size()); }
};

// CSVから区画の原型を読み込む。0は空白、1は固定、2は破壊可能。
class StageData final {
public:
    void Load(const std::filesystem::path& path);
    static std::filesystem::path DefaultPath();

    const Chunk& Start() const { return m_start; }
    const Chunk& End() const { return m_end; }
    const std::vector<Chunk>& Middle() const { return m_middle; }

    static constexpr int Columns = 9;
    static constexpr int MiddleCount = 5;

private:
    Chunk m_start;
    Chunk m_end;
    std::vector<Chunk> m_middle;
};

}
