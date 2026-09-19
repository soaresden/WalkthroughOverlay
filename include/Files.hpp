// Files.hpp — directory listing, file kinds and natural sorting ("page2" < "page10").
#pragma once
#include <string>
#include <vector>

namespace files {
    enum class Kind { Dir, Text, Image, Other };

    struct Entry {
        std::string path;   // full path, directories end with '/'
        std::string name;   // display name (directories end with '/')
        Kind kind;
    };

    Kind kindOf(const std::string& path);
    bool isText(const std::string& path);
    bool isImage(const std::string& path);

    // Lists a directory: folders first, then supported files, each group naturally sorted.
    // Unsupported files are skipped unless includeOther is set.
    std::vector<Entry> list(const std::string& dir, bool includeOther = false);

    // Only the image files of a directory, naturally sorted — used for page navigation.
    std::vector<std::string> imagesIn(const std::string& dir);

    bool exists(const std::string& path);
    bool isDirectory(const std::string& path);
    void mkdirs(const std::string& dirPath);

    std::string fileName(const std::string& path);   // "a/b/c.txt" -> "c.txt"
    std::string parentDir(const std::string& path);  // "a/b/c.txt" -> "a/b/",  "a/b/" -> "a/"
    std::string lowerExt(const std::string& path);   // ".TXT" -> ".txt"

    // Case-insensitive natural comparison (digits compared numerically)
    bool naturalLess(const std::string& a, const std::string& b);
}
