// Memory.hpp — how much overlay heap do we have, and how much is still free?
//
// An overlay's heap is fixed at load time by nx-ovlloader(+) (4/6/8 MB or a custom
// value chosen from Ultrahand's settings). Because libnx maps the whole heap up
// front, svcGetInfo() can't tell us what malloc has actually handed out — newlib's
// mallinfo() can. That is what the image loader uses to pick a safe decode size.
#pragma once
#include <switch.h>
#include <string>

namespace mem {
    struct Info {
        u64 heapTotal;   // bytes the loader gave us
        u64 heapUsed;    // bytes currently handed out by malloc
        u64 heapFree;    // total - used (fragmentation ignored)
    };

    Info info();

    // Free bytes minus a safety margin for libultrahand's own allocations
    // (glyph cache, notifications, framebuffer bookkeeping...). Never negative.
    u64 budget(u64 safetyMargin = 640 * 1024);

    std::string prettyMB(u64 bytes);   // "3.2"
    std::string summary();             // "2.1 / 8 MB"
}
