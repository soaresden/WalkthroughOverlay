#include "ImageLoader.hpp"
#include "TextDocument.hpp"
#include "Files.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>

static void writePPM(const Image& img, const char* path) {
    FILE* f = fopen(path, "wb");
    fprintf(f, "P6\n%u %u\n255\n", img.w, img.h);
    for (u32 i = 0; i < img.w * img.h; ++i) {
        u16 p = img.px[i];
        u8 rgb[3] = { (u8)((p & 0xF) * 17), (u8)(((p >> 4) & 0xF) * 17), (u8)(((p >> 8) & 0xF) * 17) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

int main(int argc, char** argv) {
    // --- natural sort ---
    assert(files::naturalLess("page2.png", "page10.png"));
    assert(!files::naturalLess("page10.png", "page2.png"));
    assert(files::naturalLess("Page002.png", "page10.png"));
    assert(files::naturalLess("a.png", "b.png"));
    assert(!files::naturalLess("abc", "abc"));
    assert(files::naturalLess("ab", "abc"));
    assert(files::fileName("sdmc:/a/b/c.txt") == "c.txt");
    assert(files::fileName("sdmc:/a/b/") == "b");
    assert(files::parentDir("sdmc:/a/b/c.txt") == "sdmc:/a/b/");
    assert(files::parentDir("sdmc:/a/b/") == "sdmc:/a/");
    assert(files::lowerExt("X.PNG") == ".png");
    printf("files: ok\n");

    // --- images ---
    const char* imgs[] = { "t_rgb.png", "t_pal.png", "t_gray16.png", "t_inter.png", "t_base.jpg", "t_prog.jpg", "t_big.jpg", "t_big.png", "t_bad.png" };
    for (auto* n : imgs) {
        Image img;
        u64 budgets[] = { 8ull << 20, 1ull << 20, 200ull << 10 };
        for (u64 b : budgets) {
            bool ok = image::load(std::string("data/") + n, 896, 4096, b, img);
            printf("%-12s budget=%6lluKB -> %s  %ux%u (src %ux%u, f=%u, %lluKB) %s\n", n, (unsigned long long)(b >> 10),
                   ok ? "OK " : "ERR", img.w, img.h, img.srcW, img.srcH, img.factor, (unsigned long long)(img.bytes() >> 10), img.error.c_str());
            if (ok && b == (8ull << 20)) { std::string out = std::string("out/") + n + ".ppm"; writePPM(img, out.c_str()); }
        }
    }

    // --- text ---
    TextDocument doc;
    assert(doc.open("data/guide.txt"));
    printf("text: %u lines, %llu bytes\n", doc.lineCount(), (unsigned long long)doc.byteSize());
    // compare against a reference read
    std::vector<std::string> ref;
    { FILE* f = fopen("data/guide.txt", "rb"); char buf[8192]; std::string cur; int c;
      while ((c = fgetc(f)) != EOF) { if (c == '\n') { ref.push_back(cur); cur.clear(); } else if (c != '\r') cur += (char)c; }
      if (!cur.empty()) ref.push_back(cur); fclose(f); }
    assert(ref.size() == doc.lineCount());
    // random access pattern: forward, backward, jumps
    u32 checks[] = { 0, 1, 63, 64, 65, 500, 511, 512, 513, 1000, 5000, doc.lineCount() - 1, 10, 3000, 2 };
    for (u32 n : checks) {
        std::string expect = ref[n];
        // expand tabs like sanitize does
        std::string e2; size_t col = 0;
        for (char ch : expect) { if (ch == '\t') { size_t s = 4 - col % 4; e2.append(s, ' '); col += s; } else { e2 += ch; ++col; } }
        if (doc.line(n) != e2) { printf("MISMATCH line %u:\n  got: [%s]\n  exp: [%s]\n", n, doc.line(n).c_str(), e2.c_str()); return 1; }
    }
    for (u32 n = 0; n < doc.lineCount(); ++n) { (void)doc.line(n); }
    for (u32 n = doc.lineCount(); n-- > 0;) { (void)doc.line(n); }
    printf("text: ok\n");

    // latin-1 file
    TextDocument l1; assert(l1.open("data/latin1.txt"));
    printf("latin1: [%s]\n", l1.line(0).c_str());
    TextDocument empty; assert(empty.open("data/empty.txt")); printf("empty: %u lines\n", empty.lineCount());
    TextDocument nonl; assert(nonl.open("data/nonl.txt")); printf("nonl: %u lines last=[%s]\n", nonl.lineCount(), nonl.line(nonl.lineCount()-1).c_str());
    return 0;
}
