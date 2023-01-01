#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

static std::string readString(FILE* file)
{
    uint8_t length = 0;
    fread(&length, sizeof(length), 1, file);

    std::string str;
    str.resize(length, ' ');

    fread(str.data(), sizeof(char), length, file);

    return str;
}

static uint32_t readUint8(FILE* file)
{
    uint8_t value = 0;
    fread(&value, sizeof(value), 1, file);
    return value;
}

struct ShaderMapping
{
    struct Shader
    {
        std::string name;
        std::string closestHit;
        std::string anyHit;
        std::vector<std::string> parameters;
        std::vector<std::string> textures;
    };

    std::unordered_map<std::string, Shader> shaders;

    void load(const std::string& filePath)
    {
        FILE* file = fopen(filePath.c_str(), "rb");
        assert(file);

        fseek(file, 0, SEEK_END);
        const long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        while (ftell(file) < fileSize)
        {
            std::string name = readString(file);

            Shader& shader = shaders[name];

            shader.name = std::move(name);
            shader.closestHit = readString(file);
            shader.anyHit = readString(file);

            const uint32_t parameterCount = readUint8(file);

            shader.parameters.reserve(parameterCount);
            for (size_t i = 0; i < parameterCount; i++)
                shader.parameters.push_back(readString(file));

            const uint32_t textureCount = readUint8(file);

            shader.textures.reserve(textureCount);
            for (size_t i = 0; i < textureCount; i++)
                shader.textures.push_back(readString(file));
        }

        fclose(file);
    }
};
