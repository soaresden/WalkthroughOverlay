// ImageLoader.cpp
#include "ImageLoader.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csetjmp>
#include <png.h>
#include <jpeglib.h>

void Image::release() {
    free(px);
    px = nullptr;
    w = h = 0;
}

namespace {

    // ------------------------------------------------------------------
    // Box-filter downsampler: feed RGB(A)8 rows in, get RGBA4444 rows out.
    // ------------------------------------------------------------------
    class Downsampler {
    public:
        bool init(u32 srcW, u32 srcH, u32 factor, u32 channels, u16* dst, u32 dstW, u32 dstH) {
            m_srcW = srcW; m_srcH = srcH; m_f = factor; m_ch = channels;
            m_dst = dst; m_dstW = dstW; m_dstH = dstH;
            m_acc = (u32*)calloc((size_t)dstW * 3, sizeof(u32));
            m_rowsInAcc = 0; m_dstY = 0;
            return m_acc != nullptr;
        }
        ~Downsampler() { free(m_acc); }

        void pushRow(const u8* row) {
            const u32 f = m_f, ch = m_ch;
            for (u32 dx = 0; dx < m_dstW; ++dx) {
                u32 x0 = dx * f, x1 = x0 + f;
                if (x1 > m_srcW) x1 = m_srcW;
                u32 r = 0, g = 0, b = 0;
                const u8* p = row + (size_t)x0 * ch;
                for (u32 x = x0; x < x1; ++x, p += ch) { r += p[0]; g += p[1]; b += p[2]; }
                u32* a = m_acc + dx * 3;
                a[0] += r; a[1] += g; a[2] += b;
            }
            if (++m_rowsInAcc == f) flush(f);
        }

        void finish() { if (m_rowsInAcc) flush(m_rowsInAcc); }

    private:
        void flush(u32 rows) {
            if (m_dstY >= m_dstH) { m_rowsInAcc = 0; return; }
            u16* out = m_dst + (size_t)m_dstY * m_dstW;
            for (u32 dx = 0; dx < m_dstW; ++dx) {
                u32 x0 = dx * m_f, x1 = x0 + m_f;
                if (x1 > m_srcW) x1 = m_srcW;
                const u32 n = (x1 - x0) * rows;
                u32* a = m_acc + dx * 3;
                const u32 r = (a[0] / n) >> 4, g = (a[1] / n) >> 4, b = (a[2] / n) >> 4;
                out[dx] = (u16)(r | (g << 4) | (b << 8) | 0xF000); // tsl::Color bit layout (r lowest)
                a[0] = a[1] = a[2] = 0;
            }
            ++m_dstY;
            m_rowsInAcc = 0;
        }

        u32 m_srcW = 0, m_srcH = 0, m_f = 1, m_ch = 3;
        u16* m_dst = nullptr;
        u32 m_dstW = 0, m_dstH = 0;
        u32* m_acc = nullptr;
        u32 m_rowsInAcc = 0, m_dstY = 0;
    };

    inline u32 ceilDiv(u32 a, u32 b) { return (a + b - 1) / b; }

    // Smallest integer factor so the result fits maxW×maxH and the budget (0 = impossible).
    u32 chooseFactor(u32 w, u32 h, u32 maxW, u32 maxH, u64 budget) {
        for (u32 f = 1; f <= 64; ++f) {
            const u32 dw = ceilDiv(w, f), dh = ceilDiv(h, f);
            const u64 need = (u64)dw * dh * 2 + (u64)dw * 12 /*accumulator*/;
            if (dw <= maxW && dh <= maxH && need <= budget) return f;
        }
        return 0;
    }

    bool allocOutput(Image& out, u32 w, u32 h, u32 f) {
        out.release();
        out.w = ceilDiv(w, f);
        out.h = ceilDiv(h, f);
        out.factor = f;
        out.px = (u16*)malloc((size_t)out.w * out.h * 2);
        if (!out.px) { out.error = "Out of memory"; out.w = out.h = 0; return false; }
        return true;
    }

    // ------------------------------------------------------------------ PNG
    struct PngErr { jmp_buf jb; };
    void pngError(png_structp png, png_const_charp) {
        PngErr* e = (PngErr*)png_get_error_ptr(png);
        longjmp(e->jb, 1);
    }
    void pngWarn(png_structp, png_const_charp) {}

    bool loadPng(FILE* f, u32 maxW, u32 maxH, u64 budget, Image& out) {
        PngErr err;
        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &err, pngError, pngWarn);
        if (!png) { out.error = "libpng init failed"; return false; }
        png_infop info = png_create_info_struct(png);
        if (!info) { png_destroy_read_struct(&png, nullptr, nullptr); out.error = "libpng init failed"; return false; }

        // volatile: these are read after longjmp() lands back here
        u8* volatile row = nullptr;
        u8* volatile whole = nullptr;
        Downsampler ds;

        if (setjmp(err.jb)) {
            free(row); free(whole);
            png_destroy_read_struct(&png, &info, nullptr);
            if (out.error.empty()) out.error = "PNG decode failed (corrupt file or out of memory)";
            out.release();
            return false;
        }

        png_init_io(png, f);
        png_read_info(png, info);

        const u32 w = png_get_image_width(png, info);
        const u32 h = png_get_image_height(png, info);
        const int colorType = png_get_color_type(png, info);
        const int bitDepth = png_get_bit_depth(png, info);
        const bool interlaced = png_get_interlace_type(png, info) != PNG_INTERLACE_NONE;
        out.srcW = w; out.srcH = h;

        // normalise everything to RGB8 (alpha dropped: pages are drawn on a solid background)
        if (colorType == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
        if (colorType == PNG_COLOR_TYPE_GRAY && bitDepth < 8) png_set_expand_gray_1_2_4_to_8(png);
        if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
        if (bitDepth == 16) png_set_strip_16(png);
        if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
        png_set_strip_alpha(png);
        int passes = 1;
        if (interlaced) passes = png_set_interlace_handling(png); // must precede read_update_info
        png_read_update_info(png, info);

        const u32 rowBytes = (u32)png_get_rowbytes(png, info);
        const u32 channels = png_get_channels(png, info);
        if (channels != 3) { out.error = "Unexpected PNG layout"; longjmp(err.jb, 1); }

        // zlib inflate window + libpng row buffers ≈ 64 KB
        u64 budgetLeft = budget > rowBytes + 65536 ? budget - rowBytes - 65536 : 0;
        if (interlaced) {
            // Interlaced files can only be read whole. Allow it when it is small.
            const u64 wholeBytes = (u64)rowBytes * h;
            if (wholeBytes + 65536 > budget) { out.error = "Interlaced PNG too large - re-export it"; longjmp(err.jb, 1); }
            budgetLeft = budget - wholeBytes - 65536;
        }

        const u32 factor = chooseFactor(w, h, maxW, maxH, budgetLeft);
        if (!factor) { out.error = "Image too large for available memory"; longjmp(err.jb, 1); }
        if (!allocOutput(out, w, h, factor)) longjmp(err.jb, 1);
        if (!ds.init(w, h, factor, channels, out.px, out.w, out.h)) { out.error = "Out of memory"; longjmp(err.jb, 1); }

        if (!interlaced) {
            row = (u8*)malloc(rowBytes);
            if (!row) { out.error = "Out of memory"; longjmp(err.jb, 1); }
            for (u32 y = 0; y < h; ++y) {
                png_read_row(png, row, nullptr);
                ds.pushRow(row);
            }
        } else {
            whole = (u8*)malloc((size_t)rowBytes * h);
            if (!whole) { out.error = "Out of memory"; longjmp(err.jb, 1); }
            for (int p = 0; p < passes; ++p)
                for (u32 y = 0; y < h; ++y)
                    png_read_row(png, whole + (size_t)y * rowBytes, nullptr);
            for (u32 y = 0; y < h; ++y) ds.pushRow(whole + (size_t)y * rowBytes);
        }
        ds.finish();
        png_read_end(png, nullptr);

        free(row); free(whole);
        png_destroy_read_struct(&png, &info, nullptr);
        return true;
    }

    // ------------------------------------------------------------------ JPEG
    struct JpegErr {
        jpeg_error_mgr pub;
        jmp_buf jb;
    };
    void jpegErrorExit(j_common_ptr cinfo) {
        JpegErr* e = (JpegErr*)cinfo->err;
        longjmp(e->jb, 1);
    }
    void jpegSilent(j_common_ptr) {}

    bool loadJpeg(FILE* f, u32 maxW, u32 maxH, u64 budget, Image& out) {
        jpeg_decompress_struct cinfo;
        JpegErr err;
        cinfo.err = jpeg_std_error(&err.pub);
        err.pub.error_exit = jpegErrorExit;
        err.pub.output_message = jpegSilent;

        u8* volatile row = nullptr;
        Downsampler ds;

        if (setjmp(err.jb)) {
            free(row);
            jpeg_destroy_decompress(&cinfo);
            if (out.error.empty()) out.error = "JPEG decode failed (corrupt file or out of memory)";
            out.release();
            return false;
        }

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, f);
        jpeg_read_header(&cinfo, TRUE);
        out.srcW = cinfo.image_width;
        out.srcH = cinfo.image_height;

        // Progressive JPEGs need the whole coefficient buffer in memory.
        if (cinfo.progressive_mode) {
            const u64 coef = (u64)cinfo.image_width * cinfo.image_height * cinfo.num_components * 2;
            if (coef + 262144 > budget) { out.error = "Progressive JPEG too large - re-export as baseline"; longjmp(err.jb, 1); }
            budget -= coef + 262144;
        }

        // Let libjpeg shrink by 1/2, 1/4 or 1/8 during the DCT (nearly free, saves
        // memory and time), then box-filter the rest. Try every DCT scale and keep
        // the combination that lands closest to the wanted size while fitting the
        // budget — libjpeg keeps a few rows of workspace per component, so a
        // smaller DCT scale also means smaller working buffers.
        u32 denom = 0, bestTotal = 0xFFFFFFFF, bestFactor = 1;
        for (u32 d = 8; d >= 1; d /= 2) {
            const u32 dw = ceilDiv(cinfo.image_width, d), dh = ceilDiv(cinfo.image_height, d);
            const u64 work = (u64)dw * cinfo.num_components * 40 + 131072;
            if (work >= budget) continue;
            const u32 f2 = chooseFactor(dw, dh, maxW, maxH, budget - work);
            if (!f2) continue;
            const u32 total = d * f2;
            if (total < bestTotal) { bestTotal = total; denom = d; bestFactor = f2; }
        }
        if (!denom) { out.error = "Image too large for available memory"; longjmp(err.jb, 1); }

        cinfo.scale_num = 1;
        cinfo.scale_denom = denom;
        cinfo.out_color_space = JCS_RGB;
        cinfo.dct_method = JDCT_IFAST;
        cinfo.do_fancy_upsampling = FALSE;
        jpeg_calc_output_dimensions(&cinfo);

        const u32 w = cinfo.output_width, h = cinfo.output_height;
        const u32 rowBytes = w * cinfo.output_components;
        if (!allocOutput(out, w, h, bestFactor)) longjmp(err.jb, 1);
        out.factor = bestFactor * denom;
        if (!ds.init(w, h, bestFactor, cinfo.output_components, out.px, out.w, out.h)) { out.error = "Out of memory"; longjmp(err.jb, 1); }

        row = (u8*)malloc(rowBytes);
        if (!row) { out.error = "Out of memory"; longjmp(err.jb, 1); }

        jpeg_start_decompress(&cinfo);
        while (cinfo.output_scanline < cinfo.output_height) {
            JSAMPROW rows[1] = { row };
            jpeg_read_scanlines(&cinfo, rows, 1);
            ds.pushRow(row);
        }
        ds.finish();
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        free(row);
        return true;
    }
}

namespace image {
    bool load(const std::string& path, u32 maxW, u32 maxH, u64 budgetBytes, Image& out) {
        out.release();
        out.error.clear();
        out.srcW = out.srcH = 0;
        out.factor = 1;

        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { out.error = "Cannot open file"; return false; }

        u8 magic[8] = {0};
        const size_t got = fread(magic, 1, sizeof(magic), f);
        rewind(f);

        bool ok = false;
        if (got >= 8 && png_sig_cmp(magic, 0, 8) == 0)
            ok = loadPng(f, maxW, maxH, budgetBytes, out);
        else if (got >= 3 && magic[0] == 0xFF && magic[1] == 0xD8 && magic[2] == 0xFF)
            ok = loadJpeg(f, maxW, maxH, budgetBytes, out);
        else
            out.error = "Not a PNG or JPEG file";

        fclose(f);
        if (!ok) out.release();
        return ok;
    }
}
