﻿//TextReader.cpp
#include <TextReader.hpp>
#include <tesla.hpp>
#include <string.h>
#include <math.h>
#include <Config.hpp>
#include <Log.hpp>

std::string TextReaderChunk::EMPTY_STRING = "";

void TextReaderChunk::loadText(FILE* file) {
    if (m_lines != nullptr) {
        return;
    }
    else if (fseek(file, m_fileOffset, SEEK_SET)) {
        Log::log("Failed to load chunk");
    }
    else {
        m_lines = new std::vector<std::string>(MAX_SIZE, std::string());
        char buf[256];
        u32 line = 0;
        u32 eol_idx;
        while (line < MAX_SIZE && (fgets(buf, sizeof(buf), file) != nullptr)) {
            eol_idx = strcspn(buf, "\r\n");
            buf[eol_idx] = 0;
            m_lines->at(line).append(buf);
            if (eol_idx < sizeof(buf) - 1) {
                line++;
            }
        }
    }
}

void TextReaderChunk::unloadText() {
    if (m_lines != nullptr) {
        delete m_lines;
        m_lines = nullptr;
    }
}

std::string& TextReaderChunk::getLine(u32 lineOffset) const {
    if (m_lines != nullptr && lineOffset < m_lines->size())
        return m_lines->at(lineOffset);
    else
        return EMPTY_STRING;
}

TextReader::TextReader(std::string const& path)
    : m_path(path),
    m_totalLines(0),
    m_lineNum(0),
    m_chunkMid(0),
    m_loading(false),
    m_loaded(false),
    m_size(15),
    m_panx(0),
    m_debug(true)
{
    Log::log("Using Tesla's built-in font renderer");

    auto j = Config::read();
    auto resume = j["files"][m_path].find("resume");
    if (resume != j["files"][m_path].end()) {
        m_lineNum = *resume;
    }
    auto bookmarks = j["files"][m_path].find("bookmarks");
    if (bookmarks != j["files"][m_path].end()) {
        for (auto& b : *bookmarks) {
            m_bookmarks.insert((u32)b);
        }
    }
}


TextReader::~TextReader() {
    if (m_file) fclose(m_file);
}

tsl::elm::Element* TextReader::createUI() {
    auto frame = new tsl::elm::OverlayFrame("", "");
    auto reader = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer* renderer, u16 x, u16 y, u16 w, u16 h) {
        // Fond noir légèrement transparent
        renderer->fillScreen(renderer->a({ 0x0, 0x0, 0x0, 0xE }));
        if (!m_loading) {
            renderer->drawString("Loading... May take a few seconds", false, 20, 50, 16, renderer->a(0xFFFF));
            m_loading = true;
            return;
        }
        else if (!m_loaded) {
            m_file = fopen(m_path.c_str(), "r");
            if (m_file) {
                Log::log("Opened file %s", m_path.c_str());
                int c;
                u32 line = 0;
                m_chunks.push_back(TextReaderChunk(ftell(m_file)));
                while ((c = fgetc(m_file)) != EOF) {
                    if (c == '\n') {
                        ++line;
                        if (line % TextReaderChunk::MAX_SIZE == 0)
                            m_chunks.push_back(TextReaderChunk(ftell(m_file)));
                    }
                }
                m_totalLines = line + 1;
                scrollTo(m_lineNum);
            }
            else {
                Log::log("Could not open file %s", m_path.c_str());
            }
            m_loaded = true;
            return;
        }
        else if (!m_file) {
            renderer->drawString("Could not open file", false, 20, 50, 16, renderer->a(0xFFFF));
            return;
        }

        const size_t numLinesToShow = 100;
        for (u32 i = 0; i < numLinesToShow; ++i) {
            u32 chunk = (m_lineNum + i) / TextReaderChunk::MAX_SIZE;
            u32 line = (m_lineNum + i) % TextReaderChunk::MAX_SIZE;

            if (chunk < m_chunks.size()) {
                if (m_bookmarks.find(m_lineNum + i) != m_bookmarks.end()) {
                    // Utiliser une couleur plus visible pour les signets (rouge vif)
                    renderer->drawRect(0, i * m_size, tsl::cfg::FramebufferWidth, 1, renderer->a({ 0xF, 0x0, 0x0, 0xF }));
                }
                printLn(
                    m_chunks[chunk].getLine(line),
                    10 + m_panx * m_size,
                    10 + i * m_size,
                    m_size,
                    renderer);
            }
        }

        u32 progressY = m_lineNum * (tsl::cfg::FramebufferHeight - 20) / m_totalLines;
        // Indicateur de position plus visible
        renderer->drawRect(0, progressY, 2, 20, renderer->a({ 0xC, 0xC, 0xC, 0xF }));

        // Affichage des touches en bas de l'écran
        const u16 keybindY = tsl::cfg::FramebufferHeight - 73;
        const u16 lineHeight = 16;
        const u16 xOffset = 20;

        //renderer->drawString("\uE081 Scroll", false, xOffset, keybindY, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE085 Scroll Faster", false, xOffset, keybindY + lineHeight, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE086 \uE091 \uE090 Scroll Even Faster", false, xOffset, keybindY + lineHeight * 2, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE086 \uE092 \uE093 Top/Bottom", false, xOffset, keybindY + lineHeight * 3, 15, renderer->a(0xFFFF));

        const u16 xOffset2 = tsl::cfg::FramebufferWidth / 2 + 20;
        renderer->drawString("\uE082 \uE07D Font Size", false, xOffset2, keybindY, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE08B Line Start", false, xOffset2, keybindY + lineHeight * 2, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE0A3 Bookmark", false, xOffset2, keybindY + lineHeight * 3, 15, renderer->a(0xFFFF));
        renderer->drawString("\uE0A4 \uE0A5 Nav Bookmarks", false, xOffset2, keybindY + lineHeight * 4, 15, renderer->a(0xFFFF));

        if (m_debug)
            renderer->drawString(std::to_string(m_fps).c_str(), false, tsl::cfg::FramebufferWidth - 20, 10, 10, renderer->a(0xFFFF));
        });

    frame->setContent(reader);
    reader->setBoundaries(0, 0, tsl::cfg::FramebufferWidth, tsl::cfg::FramebufferHeight);
    return frame;
}

void TextReader::printLn(std::string const& text, s32 x, s32 y, u32 fontSize, tsl::gfx::Renderer* renderer) const {
    // Utiliser directement drawString de Tesla avec la couleur blanche pleine opacité
    renderer->drawString(text.c_str(), false, x, y, fontSize, renderer->a(0xFFFF));
}


bool TextReader::handleInput(u64 keysDown, u64 keysHeld, const HidTouchState& touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) {
    if (keysHeld & HidNpadButton_ZR) {
        if (keysHeld & HidNpadButton_StickLUp)
            scrollTo(0);
        if (keysHeld & HidNpadButton_StickLDown)
            scroll(m_totalLines);
        if (keysHeld & HidNpadButton_StickLLeft)
            scroll(-1000);
        if (keysHeld & HidNpadButton_StickLRight)
            scroll(1000);
    }
    else if (keysHeld & HidNpadButton_ZR) {
        if (keysHeld & HidNpadButton_StickLUp)
            scroll(-20);
        if (keysHeld & HidNpadButton_StickLDown)
            scroll(20);
        if (keysHeld & HidNpadButton_StickLLeft)
            scroll(-200);
        if (keysHeld & HidNpadButton_StickLRight)
            scroll(200);
    }
    else {
        if (keysHeld & HidNpadButton_StickLUp)
            scroll(-2);
        if (keysHeld & HidNpadButton_StickLDown)
            scroll(2);
        if (keysHeld & HidNpadButton_StickLLeft)
            scroll(-1);
        if (keysHeld & HidNpadButton_StickLRight)
            scroll(1);
    }


    if (keysHeld & HidNpadButton_StickLDown)
        scroll(1);
    if (keysHeld & HidNpadButton_StickLLeft)
        m_panx++;
    if (keysHeld & HidNpadButton_StickLUp)
        scroll(-1);
    if (keysHeld & HidNpadButton_StickLRight)
        m_panx--;
    if (keysDown & HidNpadButton_StickR)
        m_panx = 0;

    //ZoomIn and ZoomOut
    if (keysHeld & HidNpadButton_StickRUp)
        m_size++;
    if (keysHeld & HidNpadButton_StickRDown)
        m_size--;

    if (keysDown & HidNpadButton_X)
        tsl::Overlay::get()->hide();

    if (keysDown & HidNpadButton_Y)
        toggleBookmark();
    if (keysDown & HidNpadButton_L)
        previousBookmark();
    if (keysDown & HidNpadButton_R)
        nextBookmark();

    if (keysDown & HidNpadButton_B)
        close();

    if (keysDown & HidNpadButton_Minus)
        m_debug = !m_debug;

    return true;
}

void TextReader::scrollTo(u32 line) {
    scroll(line - m_lineNum);
}

void TextReader::scroll(s32 offset) {
    u32 newLineNum = std::clamp((s32)m_lineNum + offset, 0, (s32)m_totalLines);
    u32 newChunk = newLineNum / TextReaderChunk::MAX_SIZE;
    u32 newOffset = newLineNum % TextReaderChunk::MAX_SIZE;

    // upwards
    if (newChunk < m_chunkMid &&
        (newChunk < m_chunkMid - 1 || newOffset < TextReaderChunk::MAX_SIZE / 2))
    {
        for (u32 chunk = m_chunkMid + 1; chunk > newChunk + 1; --chunk) {
            unloadText(chunk);
        }
        m_chunkMid = newChunk;
    }
    // downwards
    else if (newChunk > m_chunkMid &&
        (newChunk > m_chunkMid + 1 || newOffset > TextReaderChunk::MAX_SIZE / 2))
    {
        for (u32 chunk = std::max(0, (s32)m_chunkMid - 1); chunk < newChunk - 1; ++chunk) {
            unloadText(chunk);
        }
        m_chunkMid = newChunk;
    }

    loadText(m_chunkMid);
    if (m_chunkMid > 0)
        loadText(m_chunkMid - 1);
    if (m_chunkMid < m_chunks.size() - 1)
        loadText(m_chunkMid + 1);

    m_lineNum = newLineNum;
}

void TextReader::loadText(u32 chunk) {
    if (chunk < m_chunks.size()) {
        m_chunks[chunk].loadText(m_file);
    }
}

void TextReader::unloadText(u32 chunk) {
    if (chunk < m_chunks.size()) {
        m_chunks[chunk].unloadText();
    }
}

void TextReader::toggleBookmark() {
    if (m_bookmarks.find(m_lineNum) == m_bookmarks.end())
        m_bookmarks.insert(m_lineNum);
    else
        m_bookmarks.erase(m_lineNum);

    Config::update([this](nlohmann::json& j) {
        j["files"][m_path]["bookmarks"] = m_bookmarks;
        });
}

void TextReader::previousBookmark() {
    u32 prev;
    bool any = false;
    for (auto b : m_bookmarks) {
        if (b >= m_lineNum) {
            if (any)
                scrollTo(prev);
            break;
        }
        prev = b;
        any = true;
    }
    if (any)
        scrollTo(prev);
}

void TextReader::nextBookmark() {
    for (auto b : m_bookmarks) {
        if (b > m_lineNum) {
            scrollTo(b);
            break;
        }
    }
}

void TextReader::close() const {
    Config::update([this](nlohmann::json& j) {
        j["files"][m_path]["resume"] = m_lineNum;
        });
    tsl::goBack();
}