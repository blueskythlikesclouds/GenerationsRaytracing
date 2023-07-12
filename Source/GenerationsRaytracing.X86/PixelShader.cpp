#include "PixelShader.h"

static std::atomic<uint32_t> s_curId = 0;

PixelShader::PixelShader() : m_id(++s_curId)
{
}

uint32_t PixelShader::getId() const
{
    return m_id;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, PixelShader::GetDevice, Device** ppDevice)

FUNCTION_STUB(HRESULT, E_NOTIMPL, PixelShader::GetFunction, void*, UINT* pSizeOfData)