#include "ShaderLibrary.h"

#include "File.h"

ShaderLibrary::ShaderLibrary(const std::string& directoryPath)
{
	byteCode = readAllBytes(directoryPath + "/ShaderLibrary.cso", byteSize);
	shaderMapping.load(directoryPath + "/ShaderMapping.bin");
}
