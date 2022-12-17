#pragma once

#include "Resource.h"

class BaseTexture : public Resource
{
public:
    virtual DWORD SetLOD(DWORD LODNew) final;
    virtual DWORD GetLOD() final;
    virtual DWORD GetLevelCount() final;
    virtual HRESULT SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) final;
    virtual D3DTEXTUREFILTERTYPE GetAutoGenFilterType() final;
    virtual void GenerateMipSubLevels() final;
    virtual void FUN_48() final;
    virtual HRESULT FUN_4C(void* A1) final;
};
