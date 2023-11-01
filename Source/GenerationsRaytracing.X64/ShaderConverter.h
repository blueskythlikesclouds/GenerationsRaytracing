#pragma once

class ShaderConverter
{
protected:
    HMODULE m_unmanagedLibrary = nullptr;

    void* (*m_convertShaderFunction)(const void*, int, int&) = nullptr;
    void* (*m_getPointerFromHandleFunction)(void*) = nullptr;
    void (*m_freeHandleFunction)(void*) = nullptr;

public:
    ~ShaderConverter();

    void loadUnmanagedLibrary();
    void* convertShader(const void* data, uint32_t dataSize, uint32_t& convertedSize) const;
    void* getPointerFromHandle(void* handle) const;
    void freeHandle(void* handle) const;
};