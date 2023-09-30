#include "Sky.h"

void Sky::init()
{
    // Why does the game render the sky to a texture that never gets used?
    // It causes a transition barrier error so let's just skip it...
    WRITE_MEMORY(0x13DDB20, uint32_t, 0x00);
}