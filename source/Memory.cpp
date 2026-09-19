// Memory.cpp
#include "Memory.hpp"
#include <malloc.h>
#include <cstdio>

extern "C" {
    // libnx: size of the heap region requested at startup (0 = libnx default)
    extern size_t __nx_heap_size;
}

namespace mem {

    Info info() {
        Info out{};
        u64 total = 0;
        if (envHasHeapOverride())
            total = envGetHeapOverrideSize();
        if (total == 0 && __nx_heap_size != 0)
            total = __nx_heap_size;
        if (total == 0) {
            // Fall back to what the kernel reports for the whole process
            u64 t = 0;
            svcGetInfo(&t, InfoType_TotalMemorySize, CUR_PROCESS_HANDLE, 0);
            total = t;
        }

        struct mallinfo mi = mallinfo();
        out.heapTotal = total;
        out.heapUsed  = (u64)mi.uordblks;
        out.heapFree  = (out.heapTotal > out.heapUsed) ? out.heapTotal - out.heapUsed : 0;
        return out;
    }

    u64 budget(u64 safetyMargin) {
        Info i = info();
        return i.heapFree > safetyMargin ? i.heapFree - safetyMargin : 0;
    }

    std::string prettyMB(u64 bytes) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f", bytes / (1024.0 * 1024.0));
        return buf;
    }

    std::string summary() {
        Info i = info();
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f / %.0f MB", i.heapUsed / 1048576.0, i.heapTotal / 1048576.0);
        return buf;
    }
}
