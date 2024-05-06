#include "SampleChunkResource.h"

struct SampleChunkNode
{
    uint32_t flagsAndSize;
    uint32_t value;
    char name[8];
};

HOOK(void, __fastcall, SampleChunkResourceResolvePointer, 0x732E50, void* This)
{
    const auto headerV2 = reinterpret_cast<SampleChunkHeaderV2*>(This);

    SampleChunkResource::s_optimizedVertexFormat = false;
    SampleChunkResource::s_triangleTopology = false;

    if (_byteswap_ulong(headerV2->fileSize) & 0x80000000)
    {
        const uint32_t* offsets = reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(This) + _byteswap_ulong(headerV2->relocationTable));

        for (size_t i = 0; i < _byteswap_ulong(headerV2->offsetCount); i++)
        {
            uint32_t* offset = reinterpret_cast<uint32_t*>(
                reinterpret_cast<uint8_t*>(headerV2 + 1) + _byteswap_ulong(*(offsets + i)));

            *offset = reinterpret_cast<uint32_t>(headerV2 + 1) + _byteswap_ulong(*offset);
        }

        auto node = reinterpret_cast<SampleChunkNode*>(headerV2 + 1);
        while (strncmp(node->name, "Contexts", sizeof(node->name)) != 0)
        {
            if (strncmp(node->name, "GensRT  ", sizeof(node->name)) == 0)
                SampleChunkResource::s_optimizedVertexFormat = (_byteswap_ulong(node->value) == 1u);

            else if (strncmp(node->name, "Topology", sizeof(node->name)) == 0)
                SampleChunkResource::s_triangleTopology = (_byteswap_ulong(node->value) == 3u);

            ++node;
        }

        auto headerV1 = reinterpret_cast<SampleChunkHeaderV1*>(This);

        headerV1->version = _byteswap_ulong(node->value);
        headerV1->data = _byteswap_ulong(reinterpret_cast<uint8_t*>(node + 1) - reinterpret_cast<uint8_t*>(This));
    }
    else
    {
        originalSampleChunkResourceResolvePointer(This);
    }
}

void SampleChunkResource::init()
{
    INSTALL_HOOK(SampleChunkResourceResolvePointer);
}
