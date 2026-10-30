#include "StageData.h"

#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <queue>
#include <stdexcept>

namespace downwell {

namespace {

constexpr DWORD ExecutablePathCapacity = 32768;
constexpr int MaxChunkRows = 128;
constexpr int CandidateCount = 10;
constexpr int MaxSearchDepth = 7;

// ステージデータの場所とファイル名は、ここでまとめて定義する。
constexpr char StageDataDirectory[] = "Data/stage_chunks";
constexpr char StartChunkFileName[] = "start.csv";
constexpr char EndChunkFileName[] = "end.csv";
constexpr char CsvExtension[] = ".csv";
constexpr char ProjectFileName[] = "Project1.sln";

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string Trim(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

Chunk ReadChunk(const std::filesystem::path& path, bool middle) {
    Chunk chunk;
    chunk.id = path.stem().u8string();
    chunk.name = chunk.id;
    std::ifstream file(path, std::ios::binary);
    Require(static_cast<bool>(file), "Cannot open " + path.u8string());

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        // UTF-8のBOMと、表計算ソフトが付ける引用符も受け付ける。
        if (lineNumber == 1 && line.compare(0, 3, "\xef\xbb\xbf") == 0) line.erase(0, 3);
        if (Trim(line).empty()) continue;

        const std::string location = path.filename().u8string() + ": line " + std::to_string(lineNumber);
        std::vector<int> cells;
        std::size_t offset = 0;
        while (true) {
            const auto comma = line.find(',', offset);
            std::string cell = Trim(line.substr(offset, comma - offset));
            if (cell.size() >= 2 && cell.front() == '"' && cell.back() == '"') {
                cell = cell.substr(1, cell.size() - 2);
            }
            Require(cell.size() == 1 && cell[0] >= '0' && cell[0] <= '2',
                location + ": cells must be 0, 1 or 2");
            cells.push_back(cell[0] - '0');
            Require(cells.size() <= StageData::Columns, location + ": every row must have 9 cells");
            if (comma == std::string::npos) break;
            offset = comma + 1;
        }
        Require(cells.size() == StageData::Columns, location + ": every row must have 9 cells");
        // 9列は井戸の内側だけ。両側の半分の壁は含めない。
        chunk.cells.push_back(std::move(cells));
        Require(chunk.Rows() <= MaxChunkRows, chunk.id + ": row count must be between 1 and 128");
    }
    Require(!file.bad(), "Cannot read " + path.u8string());
    Require(!chunk.cells.empty(), chunk.id + ": row count must be between 1 and 128");
    const int rows = chunk.Rows();

    if (middle) {
        // 横移動と下方向だけで、入口から出口へ通れる空白があるか調べる。
        std::vector<std::vector<bool>> visited(rows, std::vector<bool>(StageData::Columns));
        std::queue<std::pair<int, int>> pending;
        for (int column = 0; column < StageData::Columns; ++column) {
            if (chunk.cells.front()[column] != 0) continue;
            pending.push({ 0, column });
            visited[0][column] = true;
        }
        bool connected = false;
        const int rowOffsets[] = { 0, 0, 1 };
        const int columnOffsets[] = { -1, 1, 0 };
        while (!pending.empty()) {
            const auto cell = pending.front();
            pending.pop();
            if (cell.first == rows - 1) connected = true;
            for (int direction = 0; direction < 3; ++direction) {
                const int row = cell.first + rowOffsets[direction];
                const int column = cell.second + columnOffsets[direction];
                if (row >= rows || column < 0 || column >= StageData::Columns) continue;
                if (visited[row][column] || chunk.cells[row][column] != 0) continue;
                visited[row][column] = true;
                pending.push({ row, column });
            }
        }
        Require(connected, chunk.id + ": no open route from top to bottom");
    }
    return chunk;
}

}

std::filesystem::path StageData::DefaultPath() {
    // 起動時の作業フォルダーに頼らず、exeの位置からDataを探す。
    // 長いパスのバッファーはヒープに置き、スタックの使用量を抑える。
    std::wstring executable(ExecutablePathCapacity, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, executable.data(), ExecutablePathCapacity);
    if (length == 0 || length >= ExecutablePathCapacity) throw std::runtime_error("Cannot locate executable");
    executable.resize(length);
    auto folder = std::filesystem::path(executable).parent_path();
    std::filesystem::path fallback;
    const std::string notFound = std::string(StageDataDirectory) + " was not found";

    for (int depth = 0; depth < MaxSearchDepth; ++depth) {
        const auto path = folder / StageDataDirectory;
        if (std::filesystem::is_regular_file(folder / ProjectFileName)) {
            Require(std::filesystem::is_directory(path), notFound);
            return path;
        }
        if (fallback.empty() && std::filesystem::is_directory(path)) fallback = path;
        const auto parent = folder.parent_path();
        if (parent == folder) break;
        folder = parent;
    }
    if (!fallback.empty()) return fallback;
    throw std::runtime_error(notFound);
}

void StageData::Load(const std::filesystem::path& path) {
    try {
        Chunk start = ReadChunk(path / StartChunkFileName, false);
        Chunk end = ReadChunk(path / EndChunkFileName, false);
        std::vector<std::filesystem::path> candidates;
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (!entry.is_regular_file() || entry.path().extension() != CsvExtension) continue;
            const auto id = entry.path().stem().u8string();
            if (id != start.id && id != end.id) candidates.push_back(entry.path());
        }
        Require(candidates.size() == CandidateCount, "middle must contain 10 CSV candidates");
        // ファイル列挙の順番に左右されないよう、名前順で読み込む。
        std::sort(candidates.begin(), candidates.end());
        std::vector<Chunk> middle;
        for (const auto& candidate : candidates) middle.push_back(ReadChunk(candidate, true));
        Require(start.cells.front()[Columns / 2] == 0,
            "Start chunk must have an empty cell at the top center for entry");

        // 全部の確認に通ったデータだけを採用する。
        m_start = std::move(start);
        m_end = std::move(end);
        m_middle = std::move(middle);
    } catch (const std::exception& error) {
        throw std::runtime_error("Stage CSV: " + path.u8string() + "\n" + error.what());
    }
}

}
