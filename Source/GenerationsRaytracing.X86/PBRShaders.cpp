#include "PBRShaders.h"

void PBRShaders::init()
{
    auto module = GetModuleHandle(TEXT("BetterFxPipeline.dll"));
    if (module != nullptr)
    {
        auto procAddress = GetProcAddress(module, "PostInit");
        if (procAddress != nullptr)
            WRITE_MEMORY(procAddress, uint8_t, 0xC3);
    }
}