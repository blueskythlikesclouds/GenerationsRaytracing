#include "Raytracing.h"

void Raytracing::init()
{
    WRITE_MEMORY(0x13DDFD8, size_t, 0); // Disable shadow map
    WRITE_MEMORY(0x13DDB20, size_t, 0); // Disable sky render
    WRITE_MEMORY(0x13DDBA0, size_t, 0); // Disable reflection map 1
    WRITE_MEMORY(0x13DDC20, size_t, 0); // Disable reflection map 2
}