#include "ShaderLibrary.h"

#include "App.h"
#include "File.h"

#include "ShaderCopyLuminancePS.h"
#include "ShaderCopyPS.h"
#include "ShaderCopyVS.h"
#include "ShaderLerpLuminancePS.h"
#include "ShaderToneMapPS.h"

ShaderLibrary::ShaderLibrary(const App& app, const std::string& directoryPath)
{
    byteCode = readAllBytes(directoryPath + "/ShaderLibrary.cso", byteSize);
    shaderMapping.load(directoryPath + "/ShaderMapping.bin");

    handle = app.device.nvrhi->createShaderLibrary(byteCode.get(), byteSize);
    copyVS = app.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Vertex), SHADER_COPY_VS, sizeof(SHADER_COPY_VS));
    copyPS = app.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_COPY_PS, sizeof(SHADER_COPY_PS));
    lerpLuminancePS = app.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_LERP_LUMINANCE_PS, sizeof(SHADER_LERP_LUMINANCE_PS));
    copyLuminancePS = app.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_COPY_LUMINANCE_PS, sizeof(SHADER_COPY_LUMINANCE_PS));
    toneMapPS = app.device.nvrhi->createShader(nvrhi::ShaderDesc(nvrhi::ShaderType::Pixel), SHADER_TONE_MAP_PS, sizeof(SHADER_TONE_MAP_PS));
}
