#include "HalfPixel.h"

void HalfPixel::init()
{
    // Prevent half-pixel correction
    WRITE_MEMORY(0x64F4C7, uintptr_t, 0x15C5858); // MTFx
    WRITE_MEMORY(0x64CC4B, uintptr_t, 0x15C5858); // ^^
    WRITE_MEMORY(0x7866E2, uintptr_t, 0x15C5858); // FxPipeline
    WRITE_MEMORY(0x64CBBB, uint8_t, 0xD9, 0xEE, 0x90, 0x90, 0x90, 0x90); // Devil's Details
    WRITE_CALL(0x64CF9E, 0x64F470);
}