#pragma once

class IndexBuffer;

class MeshDataEx : public Hedgehog::Mirage::CMeshData
{
public:
    ComPtr<IndexBuffer> m_indices;
    uint32_t m_indexCount;
};

struct MeshData
{
    static void init();
};