#include "ShaderConverter.h"

ShaderConverter::~ShaderConverter()
{
    if (m_unmanagedLibrary != nullptr)
        FreeLibrary(m_unmanagedLibrary);
}

template<typename T>
static void getProcAddress(T& function, HMODULE unmanagedLibrary, const char* functionName)
{
    function = reinterpret_cast<T>(GetProcAddress(unmanagedLibrary, functionName));
}

void ShaderConverter::loadUnmanagedLibrary()
{
    if (m_unmanagedLibrary == nullptr)
    {
        m_unmanagedLibrary = LoadLibraryA("GenerationsRaytracing.ShaderConverter.dll");
        assert(m_unmanagedLibrary != nullptr);

        getProcAddress(m_convertShaderFunction, m_unmanagedLibrary, "ConvertShader");
        getProcAddress(m_getPointerFromHandleFunction, m_unmanagedLibrary, "GetPointerFromHandle");
        getProcAddress(m_freeHandleFunction, m_unmanagedLibrary, "FreeHandle");
    }
}

void* ShaderConverter::convertShader(const void* data, uint32_t dataSize, uint32_t& convertedSize) const
{
    assert(m_unmanagedLibrary != nullptr && m_convertShaderFunction != nullptr);
    return m_convertShaderFunction(data, static_cast<int>(dataSize), reinterpret_cast<int&>(convertedSize));
}

void* ShaderConverter::getPointerFromHandle(void* handle) const
{
    assert(m_unmanagedLibrary != nullptr && m_getPointerFromHandleFunction != nullptr);
    return m_getPointerFromHandleFunction(handle);
}

void ShaderConverter::freeHandle(void* handle) const
{
    assert(m_unmanagedLibrary != nullptr && m_freeHandleFunction != nullptr);
    m_freeHandleFunction(handle);
}
