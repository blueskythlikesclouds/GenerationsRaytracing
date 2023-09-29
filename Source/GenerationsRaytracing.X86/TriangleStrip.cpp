#include "TriangleStrip.h"

void TriangleStrip::init()
{
    // Prevent conversion of restart indices to degenerate triangles
    WRITE_MEMORY(0x72EAEB, uint8_t, 0xEB);
    WRITE_MEMORY(0x736A18, uint8_t, 0xEB);
    WRITE_MEMORY(0x736B6B, uint8_t, 0xEB);
    WRITE_MEMORY(0x744B8B, uint8_t, 0xEB);
    WRITE_MEMORY(0x744E4B, uint8_t, 0xEB);
    WRITE_MEMORY(0x745027, uint8_t, 0xEB);

    // A big block of memory is allocated for degenerate triangles,
    // use the original index data instead

    WRITE_MEMORY(0x72EABC, uint8_t, 0x8B, 0x46, 0x08, 0x90, 0x90); // Move the original index data instead of allocating
    WRITE_NOP(0x72EB53, 5); // Nop the deallocation

    WRITE_MEMORY(0x7369EB, uint8_t, 0x8B, 0x43, 0x0C, 0x90, 0x90);
    WRITE_NOP(0x736AB4, 5);

    WRITE_MEMORY(0x736B3E, uint8_t, 0x8B, 0x47, 0x0C, 0x90, 0x90);
    WRITE_NOP(0x736C0E, 5);

    WRITE_MEMORY(0x744B5C, uint8_t, 0x8B, 0x47, 0x08, 0x90, 0x90);
    WRITE_NOP(0x744C41, 5);

    WRITE_MEMORY(0x744E1C, uint8_t, 0x8B, 0x47, 0x08, 0x90, 0x90);
    WRITE_NOP(0x744F01, 5);

    WRITE_MEMORY(0x744FF3, uint8_t, 0x8B, 0x43, 0x08, 0x90, 0x90);
    WRITE_NOP(0x7450D2, 5);
}