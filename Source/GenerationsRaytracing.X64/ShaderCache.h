#pragma once
#include "ShaderConverter.h"
#include "xxHashMap.h"

class ShaderCache
{
protected:
    struct Shader
    {
        const void* data;
        uint32_t dataSize;
    };

    struct ShaderHandle
    {
        XXH64_hash_t hash;
        void* handlePtr;
        uint32_t dataSize;
    };

    ShaderConverter m_shaderConverter;
    std::vector<std::unique_ptr<uint8_t[]>> m_blocks;
    xxHashMap<Shader> m_shaders;
    std::vector<ShaderHandle> m_handles;

    bool loadShaderCache(const char* name);

public:
    ShaderCache();
    ~ShaderCache();

    std::unique_ptr<uint8_t[]> getShader(const void* data, uint32_t dataSize, uint32_t& convertedSize);
    void saveShaderCache() const;
};
