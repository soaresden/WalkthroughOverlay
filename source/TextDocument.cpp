// TextDocument.cpp
#include "TextDocument.hpp"
#include <cstring>

TextDocument::~TextDocument() {
    close();
}

void TextDocument::close() {
    if (m_file) { fclose(m_file); m_file = nullptr; }
    m_blockOffsets.clear();
    m_blockOffsets.shrink_to_fit();
    m_cache.clear();
    m_cache.shrink_to_fit();
    m_cacheValid = false;
    m_lineCount = 0;
    m_size = 0;
}

bool TextDocument::open(const std::string& path) {
    close();
    m_file = fopen(path.c_str(), "rb");
    if (!m_file) return false;

    // Scan: count lines, remember where every BLOCK-th line starts.
    static constexpr size_t BUF = 32 * 1024;
    std::vector<char> buf(BUF);
    u32 line = 0;
    u64 pos = 0;
    m_blockOffsets.push_back(0);
    size_t n;
    bool lastWasNewline = true;
    while ((n = fread(buf.data(), 1, BUF, m_file)) > 0) {
        const char* p = buf.data();
        const char* end = p + n;
        while (p < end) {
            const char* nl = (const char*)memchr(p, '\n', end - p);
            if (!nl) { pos += end - p; break; }
            ++line;
            pos += (nl - p) + 1;
            if (line % BLOCK == 0) m_blockOffsets.push_back((u32)pos);
            p = nl + 1;
        }
        lastWasNewline = (buf[n - 1] == '\n');
    }
    m_size = pos;
    // A trailing partial line (no final '\n') still counts as a line.
    if (!lastWasNewline && m_size > 0) ++line;
    m_lineCount = line;
    m_blockOffsets.shrink_to_fit();
    m_cacheValid = false;
    return true;
}

void TextDocument::sanitize(std::string& s) {
    // strip CR, expand tabs, replace bytes that are not valid UTF-8 with '?'
    std::string out;
    out.reserve(s.size() + 8);
    size_t col = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        if (c == '\r') { ++i; continue; }
        if (c == '\t') {
            size_t spaces = 4 - (col % 4);
            out.append(spaces, ' ');
            col += spaces;
            ++i;
            continue;
        }
        if (c < 0x80) {
            if (c < 0x20) { ++i; continue; } // other control chars
            out += (char)c; ++col; ++i;
            continue;
        }
        // multi-byte sequence?
        size_t len = 0;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        bool valid = len > 0 && i + len <= s.size();
        for (size_t k = 1; valid && k < len; ++k)
            if (((unsigned char)s[i + k] & 0xC0) != 0x80) valid = false;
        if (valid) {
            out.append(s, i, len);
            i += len;
        } else {
            // Most likely Latin-1 (old GameFAQs guides): promote to UTF-8
            out += (char)(0xC0 | (c >> 6));
            out += (char)(0x80 | (c & 0x3F));
            ++i;
        }
        ++col;
    }
    if (out.size() > 2000) out.resize(2000);
    s.swap(out);
}

void TextDocument::loadWindow(u32 firstLine) {
    firstLine -= firstLine % BLOCK;
    u32 block = firstLine / BLOCK;
    if (block >= m_blockOffsets.size()) block = (u32)m_blockOffsets.size() - 1;
    firstLine = block * BLOCK;

    m_cache.clear();
    m_cache.reserve(WINDOW);
    m_cacheStart = firstLine;
    m_cacheValid = true;

    if (fseek(m_file, m_blockOffsets[block], SEEK_SET) != 0) return;

    char buf[1024];
    std::string cur;
    while (m_cache.size() < WINDOW) {
        cur.clear();
        bool gotAny = false, eol = false;
        while (!eol && fgets(buf, sizeof(buf), m_file)) {
            gotAny = true;
            size_t len = strlen(buf);
            if (len && buf[len - 1] == '\n') { eol = true; --len; }
            if (cur.size() < 4000) cur.append(buf, len);
        }
        if (!gotAny) break;
        sanitize(cur);
        m_cache.push_back(cur);
        if (!eol) break; // EOF without newline
    }
}

const std::string& TextDocument::line(u32 n) {
    if (!m_file || n >= m_lineCount) return m_empty;
    if (!m_cacheValid || n < m_cacheStart || n >= m_cacheStart + m_cache.size()) {
        // Center the window on the requested line so both scroll directions are cheap.
        u32 first = (n > WINDOW / 3) ? n - WINDOW / 3 : 0;
        loadWindow(first);
        if (n < m_cacheStart || n >= m_cacheStart + m_cache.size()) return m_empty;
    }
    return m_cache[n - m_cacheStart];
}
