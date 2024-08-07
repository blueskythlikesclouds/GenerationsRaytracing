#include "MeshData.h"
#include "IndexBuffer.h"
#include "Message.h"
#include "MessageSender.h"
#include "ShareVertexBuffer.h"
#include "MeshOpt.h"
#include "ModelReplacer.h"
#include "SampleChunkResource.h"
#include "Configuration.h"

HOOK(MeshDataEx*, __fastcall, MeshDataConstructor, 0x722860, MeshDataEx* This)
{
    const auto result = originalMeshDataConstructor(This);

    new (std::addressof(This->m_indices)) ComPtr<IndexBuffer>();
    This->m_indexOffset = 0;
    This->m_indexCount = 0;
    new (std::addressof(This->m_adjacency)) ComPtr<IndexBuffer>();

    return result;
}

HOOK(void, __fastcall, MeshDataDestructor, 0x7227A0, MeshDataEx* This)
{
    This->m_adjacency.~ComPtr();
    This->m_indices.~ComPtr();
    originalMeshDataDestructor(This);
}

enum DeclType
{
    DECLTYPE_FLOAT1 = 0x2C83A4,
    DECLTYPE_FLOAT2 = 0x2C23A5,
    DECLTYPE_FLOAT3 = 0x2A23B9,
    DECLTYPE_FLOAT4 = 0x1A23A6,
    DECLTYPE_D3DCOLOR = 0x182886,
    DECLTYPE_UBYTE4 = 0x1A2286,
    DECLTYPE_SHORT2 = 0x2C2359,
    DECLTYPE_SHORT4 = 0x1A235A,
    DECLTYPE_UBYTE4N = 0x1A2086,
    DECLTYPE_SHORT2N = 0x2C2159,
    DECLTYPE_SHORT4N = 0x1A215A,
    DECLTYPE_USHORT2N = 0x2C2059,
    DECLTYPE_USHORT4N = 0x1A205A,
    DECLTYPE_UDEC3 = 0x2A2287,
    DECLTYPE_UDEC3N = 0x2A2087,
    DECLTYPE_DEC3N = 0x2A2187,
    DECLTYPE_HEND3N = 0x2A2190,
    DECLTYPE_FLOAT16_2 = 0x2C235F,
    DECLTYPE_FLOAT16_4 = 0x1A2360,
    DECLTYPE_UNUSED = 0xFFFFFFFF
};

static size_t getDeclTypeSize(size_t declType)
{
    switch (declType)
    {
    case DECLTYPE_FLOAT1: return 4;
    case DECLTYPE_FLOAT2: return 8;
    case DECLTYPE_FLOAT3: return 12;
    case DECLTYPE_FLOAT4: return 16;
    case DECLTYPE_D3DCOLOR: return 4;
    case DECLTYPE_UBYTE4: return 4;
    case DECLTYPE_SHORT2: return 4;
    case DECLTYPE_SHORT4: return 8;
    case DECLTYPE_UBYTE4N: return 4;
    case DECLTYPE_SHORT2N: return 4;
    case DECLTYPE_SHORT4N: return 8;
    case DECLTYPE_USHORT2N: return 4;
    case DECLTYPE_USHORT4N: return 8;
    case DECLTYPE_UDEC3: return 4;
    case DECLTYPE_UDEC3N: return 4;
    case DECLTYPE_DEC3N: return 4;
    case DECLTYPE_HEND3N: return 4;
    case DECLTYPE_FLOAT16_2: return 4;
    case DECLTYPE_FLOAT16_4: return 8;
    }

    return 0;
}

struct VertexElement
{
    uint16_t stream;
    uint16_t offset;
    uint32_t type;
    uint8_t method;
    uint8_t usage;
    uint8_t usageIndex;
};

struct MeshResource
{
    const char* materialName;
    uint32_t indexCount;
    uint16_t* indices;
    uint32_t vertexCount;
    uint32_t vertexSize;
    uint8_t* vertexData;
    VertexElement* vertexElements;
    uint32_t nodeCount;
};

static std::vector<uint8_t> s_vertexData;

static uint32_t dec3nToUdec3n(uint32_t value)
{
    int32_t signExtend = static_cast<int32_t>((value << 22) | value);
    signExtend += 511;
    return static_cast<uint32_t>(signExtend) & 0x3FF;
}

static void optimizeVertexFormat(MeshResource* meshResource)
{
    uint32_t offsets[14][4]{};
    bool usages[14][4]{};

    VertexElement* vertexElement = meshResource->vertexElements;
    while (_byteswap_ushort(vertexElement->stream) != 0xFF && vertexElement->type != DECLTYPE_UNUSED)
    {
        offsets[vertexElement->usage][vertexElement->usageIndex] = _byteswap_ushort(vertexElement->offset);

        auto& valid = usages[vertexElement->usage][vertexElement->usageIndex];

        if (_byteswap_ulong(vertexElement->type) == DECLTYPE_FLOAT2 && vertexElement->usage == D3DDECLUSAGE_TEXCOORD && vertexElement->usageIndex != 0)
        {
            bool allZero = true;
            bool allSame = true;

            for (size_t i = 0; i < _byteswap_ulong(meshResource->vertexCount); i++)
            {
                const uint8_t* vertexData = meshResource->vertexData + i * _byteswap_ulong(meshResource->vertexSize);

                const uint32_t* texCoordN = reinterpret_cast<const uint32_t*>(vertexData + offsets[D3DDECLUSAGE_TEXCOORD][vertexElement->usageIndex]);

                if (texCoordN[0] != 0 || texCoordN[1] != 0)
                    allZero = false;

                const uint32_t* texCoord0 = reinterpret_cast<const uint32_t*>(vertexData + offsets[D3DDECLUSAGE_TEXCOORD][0]);

                if (texCoordN[0] != texCoord0[0] || texCoordN[1] != texCoord0[1])
                    allSame = false;

                valid = !allZero && !allSame;

                if (valid)
                    break;
            }
        }
        else
        {
            valid = true;
        }

        ++vertexElement;
    }

    usages[D3DDECLUSAGE_POSITION][0] = true;
    usages[D3DDECLUSAGE_COLOR][0] = true;
    usages[D3DDECLUSAGE_NORMAL][0] = true;
    usages[D3DDECLUSAGE_TANGENT][0] = true;
    usages[D3DDECLUSAGE_BINORMAL][0] = true;
    usages[D3DDECLUSAGE_TEXCOORD][0] = true;

    if (!ShareVertexBuffer::s_makingModelData || _byteswap_ulong(meshResource->nodeCount) <= 1)
    {
        usages[D3DDECLUSAGE_BLENDWEIGHT][0] = false;
        usages[D3DDECLUSAGE_BLENDINDICES][0] = false;
    }

    if (_byteswap_ulong(meshResource->nodeCount) <= 4)
    {
        usages[D3DDECLUSAGE_BLENDWEIGHT][1] = false;
        usages[D3DDECLUSAGE_BLENDINDICES][1] = false;
    }

    VertexElement elements[32]{};
    uint32_t elementIndex = 0;
    uint32_t vertexSize = 0;

    const std::pair<size_t, size_t> usageAndTypePairs[] =
    { 
        { D3DDECLUSAGE_POSITION, DECLTYPE_FLOAT3 },
        { D3DDECLUSAGE_COLOR, DECLTYPE_UBYTE4N },
        { D3DDECLUSAGE_NORMAL, DECLTYPE_UDEC3N },
        { D3DDECLUSAGE_TANGENT, DECLTYPE_UDEC3N },
        { D3DDECLUSAGE_BINORMAL, DECLTYPE_UDEC3N },
        { D3DDECLUSAGE_TEXCOORD, DECLTYPE_FLOAT16_2 },
        { D3DDECLUSAGE_BLENDINDICES, DECLTYPE_UBYTE4 },
        { D3DDECLUSAGE_BLENDWEIGHT, DECLTYPE_UBYTE4N }
    };

    for (size_t i = 0; i < 4; i++)
    {
        for (const auto [usage, type] : usageAndTypePairs)
        {
            if (usages[usage][i])
            {
                auto& element = elements[elementIndex];
                element.stream = 0;
                element.offset = _byteswap_ushort(static_cast<uint16_t>(vertexSize));
                element.type = _byteswap_ulong(type);
                element.method = D3DDECLMETHOD_DEFAULT;
                element.usage = usage;
                element.usageIndex = static_cast<uint8_t>(i);
                ++elementIndex;

                offsets[usage][i] = vertexSize;
                vertexSize += getDeclTypeSize(type);
            }
        }
    }

    if (_byteswap_ulong(meshResource->vertexSize) < vertexSize)
        MessageBox(nullptr, TEXT("Mesh optimizer panic!!!"), TEXT("Generations Raytracing"), MB_ICONERROR);

    elements[elementIndex].stream = 0xFF00;
    elements[elementIndex].type = DECLTYPE_UNUSED;
    ++elementIndex;

    s_vertexData.resize(_byteswap_ulong(meshResource->vertexCount) * vertexSize);

    for (size_t i = 0; i < _byteswap_ulong(meshResource->vertexCount); i++)
    {
        uint8_t* srcVertexData = meshResource->vertexData + i * _byteswap_ulong(meshResource->vertexSize);
        uint8_t* dstVertexData = s_vertexData.data() + i * vertexSize;

        vertexElement = meshResource->vertexElements;
        while (_byteswap_ushort(vertexElement->stream) != 0xFF && vertexElement->type != DECLTYPE_UNUSED)
        {
            if (usages[vertexElement->usage][vertexElement->usageIndex])
            {
                uint8_t* source = srcVertexData + _byteswap_ushort(vertexElement->offset);
                uint8_t* destination = dstVertexData + offsets[vertexElement->usage][vertexElement->usageIndex];

                switch (vertexElement->usage)
                {
                case D3DDECLUSAGE_POSITION:
                {
                    for (size_t j = 0; j < 3; j++)
                        reinterpret_cast<uint32_t*>(destination)[j] = _byteswap_ulong(reinterpret_cast<uint32_t*>(source)[j]);

                    break;
                }

                case D3DDECLUSAGE_NORMAL:
                case D3DDECLUSAGE_TANGENT:
                case D3DDECLUSAGE_BINORMAL:
                {
                    if (_byteswap_ulong(vertexElement->type) == DECLTYPE_FLOAT3)
                    {
                        Sonic::CNoAlignVector vector;

                        for (size_t j = 0; j < 3; j++)
                        {
                            const uint32_t value = _byteswap_ulong(reinterpret_cast<uint32_t*>(source)[j]);
                            vector[j] = *reinterpret_cast<const float*>(&value);
                        }

                        *reinterpret_cast<uint32_t*>(destination) = quantizeSnorm10(vector.normalized());
                    }
                    else
                    {
                        const uint32_t value = _byteswap_ulong(*reinterpret_cast<uint32_t*>(source));

                        *reinterpret_cast<uint32_t*>(destination) = 
                            dec3nToUdec3n(value & 0x3FF) | 
                            (dec3nToUdec3n((value >> 10) & 0x3FF) << 10) |
                            (dec3nToUdec3n((value >> 20) & 0x3FF) << 20);
                    }

                    break;
                }

                case D3DDECLUSAGE_TEXCOORD:
                {
                    if (_byteswap_ulong(vertexElement->type) == DECLTYPE_FLOAT2)
                    {
                        for (size_t j = 0; j < 2; j++)
                        {
                            const uint32_t value = _byteswap_ulong(reinterpret_cast<uint32_t*>(source)[j]);
                            reinterpret_cast<uint16_t*>(destination)[j] = quantizeHalf(*reinterpret_cast<const float*>(&value));
                        }
                    }
                    else
                    {
                        reinterpret_cast<uint16_t*>(destination)[0] = _byteswap_ushort(reinterpret_cast<uint16_t*>(source)[0]);
                        reinterpret_cast<uint16_t*>(destination)[1] = _byteswap_ushort(reinterpret_cast<uint16_t*>(source)[1]);
                    }

                    break;
                }

                case D3DDECLUSAGE_COLOR:
                {
                    if (_byteswap_ulong(vertexElement->type) == DECLTYPE_FLOAT4)
                    {
                        for (size_t j = 0; j < 4; j++)
                        {
                            const uint32_t value = _byteswap_ulong(reinterpret_cast<uint32_t*>(source)[j]);
                            destination[j] = static_cast<uint8_t>(std::clamp(static_cast<uint32_t>(*reinterpret_cast<const float*>(&value) * 255.0f), 0u, 255u));
                        }
                    }
                    else
                    {
                        *reinterpret_cast<uint32_t*>(destination) = _byteswap_ulong(*reinterpret_cast<uint32_t*>(source));
                    }

                    break;
                }

                case D3DDECLUSAGE_BLENDINDICES:
                case D3DDECLUSAGE_BLENDWEIGHT:
                {
                    *reinterpret_cast<uint32_t*>(destination) = _byteswap_ulong(*reinterpret_cast<uint32_t*>(source));
                    break;
                }
                }
            }

            ++vertexElement;
        }
    }

    meshResource->vertexSize = _byteswap_ulong(vertexSize);

    for (size_t i = 0; i < s_vertexData.size(); i += 4)
        *reinterpret_cast<uint32_t*>(meshResource->vertexData + i) = _byteswap_ulong(*reinterpret_cast<uint32_t*>(s_vertexData.data() + i));

    memcpy(meshResource->vertexElements, elements, elementIndex * sizeof(VertexElement));
}

static std::vector<uint16_t> s_indices;

static IndexBuffer* createIndexBuffer()
{
    const size_t byteSize = s_indices.size() * sizeof(uint16_t);
    const auto indexBuffer = new IndexBuffer(byteSize);

    auto& createMsg = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
    createMsg.length = byteSize;
    createMsg.format = D3DFMT_INDEX16;
    createMsg.indexBufferId = indexBuffer->getId();
    s_messageSender.endMessage();

    auto& copyMsg = s_messageSender.makeMessage<MsgWriteIndexBuffer>(byteSize);
    copyMsg.indexBufferId = indexBuffer->getId();
    copyMsg.offset = 0;
    copyMsg.initialWrite = true;
    memcpy(copyMsg.data, s_indices.data(), byteSize);
    s_messageSender.endMessage();

    s_indices.clear();

    return indexBuffer;
}

static void convertToTriangles(MeshDataEx& meshData, const MeshResource* meshResource)
{
    if (!Configuration::s_enableRaytracing)
        return;

    assert(meshData.m_indices == nullptr);
    
    const uint32_t indexNum = _byteswap_ulong(meshResource->indexCount);
    if (indexNum <= 2)
        return;
    
    meshData.m_indexOffset = s_indices.size();
    s_indices.reserve(meshData.m_indexOffset + (indexNum - 2) * 3);
    
    size_t start = 0;
    
    for (size_t i = 0; i < _byteswap_ulong(meshResource->indexCount); ++i)
    {
        if (meshResource->indices[i] == 0xFFFF)
        {
            start = i + 1;
        }
        else if (i - start >= 2)
        {
            uint16_t a = _byteswap_ushort(meshResource->indices[i - 2]);
            uint16_t b = _byteswap_ushort(meshResource->indices[i - 1]);
            uint16_t c = _byteswap_ushort(meshResource->indices[i]);
    
            if ((i - start) & 1)
                std::swap(a, b);
    
            if (a != b && a != c && b != c)
            {
                s_indices.push_back(a);
                s_indices.push_back(b);
                s_indices.push_back(c);
            }
        }
    }
    
    meshData.m_indexCount = s_indices.size() - meshData.m_indexOffset;
}

static void generateAdjacencyData(MeshDataEx& meshData, MeshResource* meshResource)
{
    if (Configuration::s_enableRaytracing && strstr(meshResource->materialName, "_smooth_normal") != nullptr)
    {
        std::vector<std::vector<uint32_t>> adjacentTriangles(_byteswap_ulong(meshResource->vertexCount));

        if (ShareVertexBuffer::s_loadingSampleChunkV2 && SampleChunkResource::s_triangleTopology)
        {
            for (size_t i = 0; i < _byteswap_ulong(meshResource->indexCount); i++)
                adjacentTriangles[_byteswap_ushort(meshResource->indices[i])].push_back(i / 3);
        }
        else
        {
            for (size_t i = 0; i < meshData.m_indexCount; i++)
                adjacentTriangles[s_indices[meshData.m_indexOffset + i]].push_back(i / 3);
        }

        size_t adjacentIndexNum = 0;
        for (const auto& adjacencyIndices : adjacentTriangles)
            adjacentIndexNum += adjacencyIndices.size();

        const size_t byteSize = (adjacentTriangles.size() * 2 + adjacentIndexNum) * sizeof(uint32_t);

        meshData.m_adjacency.Attach(new IndexBuffer(byteSize));

        auto& createMsg = s_messageSender.makeMessage<MsgCreateIndexBuffer>();
        createMsg.length = byteSize;
        createMsg.format = D3DFMT_UNKNOWN;
        createMsg.indexBufferId = meshData.m_adjacency->getId();
        s_messageSender.endMessage();

        auto& copyMsg = s_messageSender.makeMessage<MsgWriteIndexBuffer>(byteSize);
        copyMsg.indexBufferId = meshData.m_adjacency->getId();
        copyMsg.offset = 0;
        copyMsg.initialWrite = true;

        auto metaCursor = reinterpret_cast<uint32_t*>(copyMsg.data);
        auto indexCursor = metaCursor + adjacentTriangles.size() * 2;

        size_t curIndex = 0;

        for (auto& adjacencyIndices : adjacentTriangles)
        {
            *metaCursor = curIndex;
            *(metaCursor + 1) = adjacencyIndices.size();
            metaCursor += 2;

            memcpy(indexCursor, adjacencyIndices.data(), adjacencyIndices.size() * sizeof(uint32_t));
            indexCursor += adjacencyIndices.size();
            curIndex += adjacencyIndices.size();
        }

        s_messageSender.endMessage();
    }
}

HOOK(void, __fastcall, ProcessShareVertexBuffer, 0x72EBD0, Hedgehog::Mirage::CShareVertexBuffer* This)
{
    if (ShareVertexBuffer::s_loadingSampleChunkV2 && SampleChunkResource::s_triangleTopology)
    {
        originalProcessShareVertexBuffer(This);

        for (const auto& meshData : This->m_MeshDataList)
        {
            const auto meshDataEx = reinterpret_cast<MeshDataEx*>(meshData.pMeshData);

            meshDataEx->m_indices = reinterpret_cast<IndexBuffer*>(meshDataEx->m_pD3DIndexBuffer);
            meshDataEx->m_indexCount = meshDataEx->m_IndexNum;
            meshDataEx->m_indexOffset = meshDataEx->m_IndexOffset;
        }
    }
    else
    {
        if (!s_indices.empty())
        {
            ComPtr<IndexBuffer> indexBuffer;
            indexBuffer.Attach(createIndexBuffer());

            for (const auto& meshData : This->m_MeshDataList)
                reinterpret_cast<MeshDataEx*>(meshData.pMeshData)->m_indices = indexBuffer;
        }

        originalProcessShareVertexBuffer(This);
    }
}

static void optimizeMeshData(MeshDataEx& meshData, MeshResource* meshResource)
{
    if (!meshData.IsMadeOne())
    {
        if (!ShareVertexBuffer::s_loadingSampleChunkV2 || !SampleChunkResource::s_optimizedVertexFormat)
            optimizeVertexFormat(meshResource);

        if (!ShareVertexBuffer::s_loadingSampleChunkV2 || !SampleChunkResource::s_triangleTopology)
            convertToTriangles(meshData, meshResource);

        generateAdjacencyData(meshData, meshResource);
    }
}

HOOK(void, __cdecl, MakeMeshData, 0x744A00,
    MeshDataEx& meshData,
    MeshResource* meshResource,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    optimizeMeshData(meshData, meshResource);
    originalMakeMeshData(meshData, meshResource, databaseWrapper, renderingInfrastructure);
}

HOOK(void, __cdecl, MakeMeshData2, 0x744CC0,
    MeshDataEx& meshData,
    MeshResource* meshResource,
    const Hedgehog::Mirage::CMirageDatabaseWrapper& databaseWrapper,
    Hedgehog::Mirage::CRenderingInfrastructure& renderingInfrastructure)
{
    optimizeMeshData(meshData, meshResource);
    originalMakeMeshData2(meshData, meshResource, databaseWrapper, renderingInfrastructure);
}

void MeshData::init()
{
    if (Configuration::s_enableRaytracing)
    {
        WRITE_MEMORY(0x72294D, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x739511, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x739641, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x7397D1, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73C763, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73C873, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73C9EA, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73D063, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73D173, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73D2EA, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73D971, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73DA86, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73DBFE, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73E383, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73E493, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73E606, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73EF23, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73F033, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x73F1A6, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x745661, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x745771, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x7458E4, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0x745D01, uint32_t, sizeof(MeshDataEx));
        WRITE_MEMORY(0xCD9A99, uint32_t, sizeof(MeshDataEx));

        INSTALL_HOOK(MeshDataConstructor);
        INSTALL_HOOK(MeshDataDestructor);

        INSTALL_HOOK(ProcessShareVertexBuffer);
    }

    INSTALL_HOOK(MakeMeshData);
    INSTALL_HOOK(MakeMeshData2);

    WRITE_MEMORY(0x13E93DC, uint32_t, DECLTYPE_UDEC3N);
}
