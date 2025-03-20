//textreader.hpp


#pragma once
#include <tesla.hpp>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <chrono>

// Class to manage chunks of text in the reader
class TextReaderChunk {
public:
    static const size_t MAX_SIZE = 1000;
    TextReaderChunk(long int fileOffset)
        : m_fileOffset(fileOffset),
        m_lines(nullptr) {
    }
    ~TextReaderChunk() { unloadText(); }
    void loadText(FILE* file);
    void unloadText();
    std::string& getLine(u32 lineOffset) const;
private:
    static std::string EMPTY_STRING;
    long int m_fileOffset;
    std::vector<std::string>* m_lines;
};

// Main TextReader class
class TextReader : public tsl::Gui {
public:
    explicit TextReader(std::string const& path);
    ~TextReader();
    tsl::elm::Element* createUI() override;
    bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos,
        HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override;
    void scrollTo(u32 line);
    void scroll(s32 offset);
    void toggleBookmark();
    void previousBookmark();
    void nextBookmark();
    void close() const;
protected:
    void printLn(const std::string& text, s32 x, s32 y, u32 fontSize, tsl::gfx::Renderer* renderer) const;
private:
    void loadText(u32 chunk);
    void unloadText(u32 chunk);
private:
    std::string m_path;
    FILE* m_file;
    u32 m_totalLines;
    u32 m_lineNum;
    u32 m_chunkMid;
    std::vector<TextReaderChunk> m_chunks;
    bool m_loading;
    bool m_loaded;
    u32 m_size;
    s32 m_panx;
    std::set<u32> m_bookmarks;
    std::chrono::steady_clock::time_point m_timePrev;
    std::chrono::milliseconds m_timeAggregate;
    u32 m_timeTicks;
    u32 m_fps;
    bool m_debug;
};