// ImageLoader.hpp — decode PNG/JPEG straight into a screen-sized RGBA4444 buffer.
//
// Overlays get 4–8 MB of heap in total, so we never decode a whole image at full
// resolution. Rows are streamed out of libpng / libjpeg-turbo and box-filtered on
// the fly by an integer factor chosen so the result fits both the requested
// maximum size and the memory budget. A 4000×3000 photo costs a 16 KB row buffer
// plus the final (small) bitmap — nothing else.
#pragma once
#include <string>
#include <switch.h>

struct Image {
    u16* px  = nullptr;   // RGBA4444, w*h pixels, row-major, alpha always 0xF
    u32  w = 0, h = 0;    // decoded size
    u32  srcW = 0, srcH = 0; // original size in the file
    u32  factor = 1;      // total downscale factor applied
    std::string error;    // non-empty when loading failed

    Image() = default;
    ~Image() { release(); }
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void release();
    bool ok() const { return px != nullptr; }
    u64  bytes() const { return (u64)w * h * 2; }
};

namespace image {
    // Decodes `path` into `out`, never exceeding maxW×maxH pixels nor `budgetBytes`
    // of heap for the bitmap. Returns false and fills out.error on failure.
    bool load(const std::string& path, u32 maxW, u32 maxH, u64 budgetBytes, Image& out);
}
