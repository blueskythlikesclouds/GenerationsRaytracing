#include "ShaderCache.h"

#include "ShaderConverter.h"

#pragma pack(push, 1)
struct BlockHeader
{
    uint32_t version;
    uint32_t uncompressedSize;
    uint32_t compressedSize;
};

struct ShaderHeader
{
    XXH64_hash_t hash;
    uint32_t dataSize;
};
#pragma pack(pop)

bool ShaderCache::loadShaderCache(const char* name)
{
    FILE* file = fopen(name, "rb");
    if (file != nullptr)
    {
        fseek(file, 0, SEEK_END);
        const long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::vector<uint8_t> compressedBlock;
        long filePosition;

        while ((filePosition = ftell(file)) < fileSize)
        {
            BlockHeader blockHeader{};
            bool isBlockValid = false;

            if (fread(&blockHeader, sizeof(BlockHeader), 1, file) == 1 && blockHeader.uncompressedSize > 0 &&
                blockHeader.compressedSize > 0 && blockHeader.compressedSize <= static_cast<uint32_t>(fileSize - filePosition))
            {
                compressedBlock.resize(blockHeader.compressedSize);

                if (fread(compressedBlock.data(), 1, compressedBlock.size(), file) == blockHeader.compressedSize)
                {
                    auto& uncompressedBlock = m_blocks.emplace_back(new uint8_t[blockHeader.uncompressedSize]);

                    const int decompressedSize = LZ4_decompress_safe(
                        reinterpret_cast<const char*>(compressedBlock.data()), 
                        reinterpret_cast<char*>(uncompressedBlock.get()), 
                        static_cast<int>(blockHeader.compressedSize), 
                        static_cast<int>(blockHeader.uncompressedSize));

                   if (static_cast<uint32_t>(decompressedSize) == blockHeader.uncompressedSize)
                   {
                       const uint8_t* blockPtr = uncompressedBlock.get();

                       while (blockPtr < (uncompressedBlock.get() + blockHeader.uncompressedSize))
                       {
                           const ShaderHeader* shaderHeader = reinterpret_cast<const ShaderHeader*>(blockPtr);
                           blockPtr += sizeof(ShaderHeader);

                           m_shaders.emplace(shaderHeader->hash, Shader{ blockPtr, shaderHeader->dataSize });
                           blockPtr += shaderHeader->dataSize;
                       }

                       isBlockValid = true;
                   }
                }
            }

            if (!isBlockValid)
                break;
        }

        fclose(file);

        return true;
    }

    return false;
}

ShaderCache::ShaderCache()
{
    if (!loadShaderCache("pregenerated_shader_cache.bin"))
    {
        MessageBox(nullptr,
            TEXT("Unable to open \"pregenerated_shader_cache.bin\" in mod directory."),
            TEXT("Generations Raytracing"),
            MB_ICONERROR);
    }
    loadShaderCache("runtime_generated_shader_cache.bin");
}

ShaderCache::~ShaderCache()
{
    for (const auto& handle : m_handles)
        m_shaderConverter.freeHandle(handle.handlePtr);
}

std::unique_ptr<uint8_t[]> ShaderCache::getShader(const void* data, uint32_t dataSize, uint32_t& convertedSize)
{
    const XXH64_hash_t hash = XXH3_64bits(data, dataSize);
    auto& shader = m_shaders[hash];

    if (shader.data == nullptr)
    {
        m_shaderConverter.loadUnmanagedLibrary();

        auto& handle = m_handles.emplace_back();
        handle.hash = hash;
        handle.handlePtr = m_shaderConverter.convertShader(data, dataSize, handle.dataSize);

        shader.data = m_shaderConverter.getPointerFromHandle(handle.handlePtr);
        shader.dataSize = handle.dataSize;
    }

    auto convertedShader = std::make_unique<uint8_t[]>(shader.dataSize);
    memcpy(convertedShader.get(), shader.data, shader.dataSize);
    convertedSize = shader.dataSize;

    return convertedShader;
}

void ShaderCache::saveShaderCache() const
{
    if (!m_handles.empty())
    {
        FILE* file = fopen("runtime_generated_shader_cache.bin", "ab");
        if (file != nullptr)
        {
            BlockHeader blockHeader{};

            for (const auto& handle : m_handles)
                blockHeader.uncompressedSize += sizeof(ShaderHeader) + handle.dataSize;

            const auto uncompressedBlock = std::make_unique<uint8_t[]>(blockHeader.uncompressedSize);
            uint8_t* blockPtr = uncompressedBlock.get();

            for (const auto& handle : m_handles)
            {
                const auto shaderHeader = reinterpret_cast<ShaderHeader*>(blockPtr);
                shaderHeader->hash = handle.hash;
                shaderHeader->dataSize = handle.dataSize;
                blockPtr += sizeof(ShaderHeader);

                memcpy(blockPtr, m_shaderConverter.getPointerFromHandle(handle.handlePtr), handle.dataSize);
                blockPtr += handle.dataSize;
            }

            const int compressBound = LZ4_compressBound(static_cast<int>(blockHeader.uncompressedSize));
            const auto compressedBlock = std::make_unique<uint8_t[]>(compressBound);

            blockHeader.compressedSize = static_cast<uint32_t>(LZ4_compress_default(
                reinterpret_cast<const char*>(uncompressedBlock.get()),
                reinterpret_cast<char*>(compressedBlock.get()),
                static_cast<int>(blockHeader.uncompressedSize),
                compressBound));

            fwrite(&blockHeader, sizeof(BlockHeader), 1, file);
            fwrite(compressedBlock.get(), 1, blockHeader.compressedSize, file);
            fclose(file);
        }
        else
        {
            MessageBox(nullptr,
                TEXT("Unable to open \"runtime_generated_shader_cache.bin\" in mod directory."),
                TEXT("Generations Raytracing"),
                MB_ICONERROR);
        }
    }
}
