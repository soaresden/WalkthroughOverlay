// TextDocument.hpp — random access to the lines of a (possibly multi-megabyte) text
// file without ever holding the whole file in the overlay's tiny heap.
//
// On open() the file is scanned once to count lines and record the byte offset of
// every BLOCK-th line (a 5 MB / 100k-line guide costs ~6 KB of index). Lines are then
// served from a sliding window of WINDOW lines re-read from the SD card whenever the
// requested line leaves it.
#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <switch.h>

class TextDocument {
public:
    static constexpr u32 BLOCK  = 64;    // lines per index entry
    static constexpr u32 WINDOW = 256;   // lines kept in memory (multiple of BLOCK)

    TextDocument() = default;
    ~TextDocument();
    TextDocument(const TextDocument&) = delete;
    TextDocument& operator=(const TextDocument&) = delete;

    bool open(const std::string& path);
    void close();

    bool isOpen() const { return m_file != nullptr; }
    u32  lineCount() const { return m_lineCount; }
    u64  byteSize() const { return m_size; }

    // Returns the line (without EOL, tabs expanded, invalid UTF-8 scrubbed).
    // The reference stays valid until the next call that moves the window.
    const std::string& line(u32 n);

private:
    void loadWindow(u32 firstLine);
    static void sanitize(std::string& s);

    FILE* m_file = nullptr;
    u64 m_size = 0;
    u32 m_lineCount = 0;
    std::vector<u32> m_blockOffsets;   // byte offset of line (i * BLOCK)

    std::vector<std::string> m_cache;
    u32 m_cacheStart = 0;
    bool m_cacheValid = false;
    std::string m_empty;
};
