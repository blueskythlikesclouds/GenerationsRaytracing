#pragma once

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

    std::unordered_map<std::string, Shader> map;

    void load(const std::string& filePath);
};
