#include <stdint.h>

namespace android {
    extern "C" void _ZN7android13GraphicBuffer4lockEjPPvPiS3_(uint32_t inUsage, void** vaddr, int32_t* outBytesPerPixel, int32_t* outBytesPerStride);

    extern "C" void _ZN7android13GraphicBuffer4lockEjPPv(uint32_t inUsage, void** vaddr) {
        _ZN7android13GraphicBuffer4lockEjPPvPiS3_(inUsage, vaddr, nullptr, nullptr);
    }
}
