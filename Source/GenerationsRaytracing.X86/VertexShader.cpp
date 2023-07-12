#include "VertexShader.h"

static std::atomic<uint32_t> s_curId = 0;

VertexShader::VertexShader() : m_id(++s_curId)
{
}

uint32_t VertexShader::getId() const
{
    return m_id;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, VertexShader::GetDevice, Device** ppDevice)

FUNCTION_STUB(HRESULT, E_NOTIMPL, VertexShader::GetFunction, void*, UINT* pSizeOfData)