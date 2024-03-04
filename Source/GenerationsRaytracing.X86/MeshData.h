#pragma once

class IndexBuffer;

class MeshDataEx : public Hedgehog::Mirage::CMeshData
{
public:
    ComPtr<IndexBuffer> m_indices;
    uint32_t m_indexCount;
    ComPtr<IndexBuffer> m_adjacency;
};

struct MeshData
{
    static void init();
};