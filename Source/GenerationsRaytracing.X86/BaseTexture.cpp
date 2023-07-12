#include "BaseTexture.h"

#include "SubAllocator.h"

static SubAllocator s_subAllocator;

BaseTexture::BaseTexture(uint32_t levelCount)
{
    m_id = s_subAllocator.allocate();
    m_levelCount = levelCount;
}

BaseTexture::~BaseTexture()
{
    s_subAllocator.free(m_id);
}

uint32_t BaseTexture::getId() const
{
    return m_id;
}

FUNCTION_STUB(DWORD, NULL, BaseTexture::SetLOD, DWORD LODNew)

FUNCTION_STUB(DWORD, NULL, BaseTexture::GetLOD)

FUNCTION_STUB(DWORD, NULL, BaseTexture::GetLevelCount)

FUNCTION_STUB(HRESULT, E_NOTIMPL, BaseTexture::SetAutoGenFilterType, D3DTEXTUREFILTERTYPE FilterType)

FUNCTION_STUB(D3DTEXTUREFILTERTYPE, static_cast<D3DTEXTUREFILTERTYPE>(NULL), BaseTexture::GetAutoGenFilterType)

FUNCTION_STUB(void, , BaseTexture::GenerateMipSubLevels)

FUNCTION_STUB(void, , BaseTexture::FUN_48)

FUNCTION_STUB(HRESULT, E_NOTIMPL, BaseTexture::FUN_4C, void* A1)