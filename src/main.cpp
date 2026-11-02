#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace ansi {
    constexpr std::string_view reset = "\033[0m";
    constexpr std::string_view bold = "\033[1m";
    constexpr std::string_view dim = "\033[2m";
    constexpr std::string_view gray = "\033[90m";
    constexpr std::string_view blue = "\033[1;34m";
    constexpr std::string_view green = "\033[32m";
    constexpr std::string_view yellow = "\033[33m";
    constexpr std::string_view red = "\033[31m";
    constexpr std::string_view cyan = "\033[36m";
}

enum class EntryType {
    Directory,
    SkippedDirectory,
    File,
    Other
};

struct Entry {
    fs::path path;
    EntryType type{};
    std::uintmax_t size{};
};

struct ScanResult {
    std::vector<Entry> entries;
    std::unordered_map<fs::path, std::uintmax_t> directorySizes;
    std::uintmax_t totalSize{};
    std::size_t fileCount{};
    std::size_t directoryCount{};
};

struct Row {
    std::string label;
    std::size_t labelWidth{};
    std::string sizeText;
    std::string_view sizeColor = ansi::reset;
    double ratio = -1.0;
};

constexpr std::array<std::string_view, 2> skippedDirectories{
    ".git",
    "build"
};

bool isSkipped(const fs::path& path) {
    const auto name = path.filename().string();

    return std::ranges::find(skippedDirectories, name)
        != skippedDirectories.end();
}

std::string formatSize(std::uintmax_t bytes) {
    constexpr std::array<std::string_view, 5> units{
        "bytes", "KB", "MB", "GB", "TB"
    };

    double size = static_cast<double>(bytes);
    std::size_t unit = 0;

    while (size >= 1024.0 && unit + 1 < units.size()) {
        size /= 1024.0;
        ++unit;
    }

    if (unit == 0) {
        return std::format("{} {}", bytes, units[unit]);
    }

    return std::format("{:.1f} {}", size, units[unit]);
}

std::size_t displayWidth(std::string_view text) {
    std::size_t width = 0;

    for (unsigned char ch : text) {
        if ((ch & 0xC0) != 0x80) {
            ++width;
        }
    }

    return width;
}

std::string repeat(std::string_view text, std::size_t count) {
    std::string result;

    for (std::size_t i = 0; i < count; ++i) {
        result += text;
    }

    return result;
}

std::string makeBar(double ratio, std::size_t width) {
    const auto filled = static_cast<std::size_t>(
        std::round(std::clamp(ratio, 0.0, 1.0) * width)
    );

    return repeat("█", filled) + repeat("░", width - filled);
}

std::string_view colorForSize(std::uintmax_t bytes) {
    constexpr std::uintmax_t MB = 1024 * 1024;

    if (bytes >= 100 * MB) return ansi::red;
    if (bytes >= MB) return ansi::yellow;

    return ansi::green;
}

void addToParents(
    ScanResult& result,
    const fs::path& file,
    std::uintmax_t size
) {
    auto directory = file.parent_path();

    while (true) {
        result.directorySizes[directory] += size;

        if (directory.empty()) break;

        directory = directory.parent_path();
    }
}

ScanResult scan(const fs::path& root) {
    ScanResult result;
    std::error_code ec;

    fs::recursive_directory_iterator it{
        root,
        fs::directory_options::skip_permission_denied,
        ec
    };

    const fs::recursive_directory_iterator end;

    for (; !ec && it != end; it.increment(ec)) {
        const auto& entry = *it;
        const auto relativePath = entry.path().lexically_relative(root);

        std::error_code error;

        if (entry.is_symlink(error) || error) {
            result.entries.push_back({
                relativePath, EntryType::Other, 0
            });
            continue;
        }

        if (entry.is_directory(error)) {
            ++result.directoryCount;

            const bool skipped = isSkipped(entry.path());

            if (skipped) {
                it.disable_recursion_pending();
            }

            result.entries.push_back({
                relativePath,
                skipped ? EntryType::SkippedDirectory : EntryType::Directory,
                0
            });

            continue;
        }

        if (!error && entry.is_regular_file(error)) {
            const auto size = entry.file_size(error);
            const auto fileSize = error ? 0 : size;

            ++result.fileCount;
            result.totalSize += fileSize;

            addToParents(result, relativePath, fileSize);

            result.entries.push_back({
                relativePath, EntryType::File, fileSize
            });

            continue;
        }

        result.entries.push_back({
            relativePath, EntryType::Other, 0
        });
    }

    if (ec) {
        std::cerr << std::format("Scan stopped early: {}\n", ec.message());
    }

    return result;
}

void print(const ScanResult& result) {
    using EntryList = std::vector<const Entry*>;

    std::unordered_map<fs::path, EntryList> children;

    for (const auto& entry : result.entries) {
        children[entry.path.parent_path()].push_back(&entry);
    }

    for (auto& [path, entries] : children) {
        std::ranges::sort(entries, [](const Entry* a, const Entry* b) {
            const bool aDirectory =
                a->type == EntryType::Directory ||
                a->type == EntryType::SkippedDirectory;

            const bool bDirectory =
                b->type == EntryType::Directory ||
                b->type == EntryType::SkippedDirectory;

            if (aDirectory != bDirectory) return aDirectory;

            return a->path.filename() < b->path.filename();
        });
    }

    const double totalSize = std::max(
        static_cast<double>(result.totalSize),
        1.0
    );

    std::vector<Row> rows;

    const auto collect = [&](
        auto&& self,
        const fs::path& parent,
        const std::string& prefix
    ) -> void {
        const auto group = children.find(parent);

        if (group == children.end()) return;

        const auto& entries = group->second;

        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& entry = *entries[i];
            const bool last = i + 1 == entries.size();

            const std::string_view branch = last ? "└── " : "├── ";
            const std::string childPrefix = prefix + (last ? "    " : "│   ");
            const std::string name = entry.path.filename().string();

            std::string_view icon = "•";
            std::string_view color = ansi::reset;

            Row row;

            switch (entry.type) {
                case EntryType::Directory: {
                    const auto it = result.directorySizes.find(entry.path);
                    const auto size = it != result.directorySizes.end()
                        ? it->second
                        : 0;

                    icon = "▸";
                    color = ansi::blue;
                    row.sizeText = formatSize(size);
                    row.sizeColor = colorForSize(size);
                    row.ratio = static_cast<double>(size) / totalSize;
                    break;
                }

                case EntryType::SkippedDirectory:
                    icon = "⊘";
                    color = ansi::dim;
                    row.sizeText = "skipped";
                    row.sizeColor = ansi::dim;
                    break;

                case EntryType::File:
                    row.sizeText = formatSize(entry.size);
                    row.sizeColor = colorForSize(entry.size);
                    row.ratio = static_cast<double>(entry.size) / totalSize;
                    break;

                case EntryType::Other:
                    icon = "?";
                    color = ansi::cyan;
                    break;
            }

            row.label = std::format(
                "{}{}{}{}{}{} {}{}",
                ansi::gray,
                prefix,
                branch,
                ansi::reset,
                color,
                icon,
                name,
                ansi::reset
            );

            row.labelWidth =
                displayWidth(prefix) +
                displayWidth(branch) +
                displayWidth(icon) +
                1 +
                displayWidth(name);

            rows.push_back(std::move(row));

            if (entry.type == EntryType::Directory) {
                self(self, entry.path, childPrefix);
            }
        }
    };

    collect(collect, fs::path{}, "");

    constexpr std::size_t barWidth = 12;

    std::size_t labelMax = 0;
    std::size_t sizeMax = 0;

    for (const auto& row : rows) {
        labelMax = std::max(labelMax, row.labelWidth);
        sizeMax = std::max(sizeMax, row.sizeText.size());
    }

    for (const auto& row : rows) {
        std::cout << row.label
                  << ' '
                  << ansi::gray
                  << repeat("·", labelMax - row.labelWidth + 1)
                  << ansi::reset
                  << ' '
                  << std::string(sizeMax - row.sizeText.size(), ' ')
                  << row.sizeColor
                  << row.sizeText
                  << ansi::reset;

        if (row.ratio >= 0.0) {
            std::cout << std::format(
                "  {}{}{} {:5.1f}%",
                row.sizeColor,
                makeBar(row.ratio, barWidth),
                ansi::reset,
                row.ratio * 100.0
            );
        }

        std::cout << '\n';
    }

    std::cout << '\n'
              << ansi::gray
              << repeat("─", labelMax + sizeMax + barWidth + 12)
              << ansi::reset
              << '\n'
              << std::format(
                  "{}Directories{} {}\n",
                  ansi::bold, ansi::reset, result.directoryCount
              )
              << std::format(
                  "{}Files      {} {}\n",
                  ansi::bold, ansi::reset, result.fileCount
              )
              << std::format(
                  "{}Total size {} {}{}{}\n",
                  ansi::bold,
                  ansi::reset,
                  ansi::green,
                  formatSize(result.totalSize),
                  ansi::reset
              );
}

int main(int argc, char* argv[]) {
    std::error_code ec;

    const fs::path requested =
        argc > 1 ? fs::path{argv[1]} : fs::current_path(ec);

    if (ec) {
        std::cerr << std::format("Cannot get current path: {}\n", ec.message());
        return 1;
    }

    const fs::path root = fs::canonical(requested, ec);

    if (ec) {
        std::cerr << std::format(
            "Cannot open '{}': {}\n",
            requested.string(),
            ec.message()
        );
        return 1;
    }

    if (!fs::is_directory(root, ec)) {
        std::cerr << std::format("'{}' is not a directory\n", root.string());
        return 1;
    }

    std::cout << ansi::bold
              << "📂 " << root.string()
              << ansi::reset << "\n\n";

    const auto result = scan(root);
    print(result);
}
