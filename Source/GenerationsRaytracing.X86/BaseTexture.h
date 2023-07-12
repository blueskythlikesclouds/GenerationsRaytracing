#pragma once

#include "Resource.h"

class BaseTexture : public Resource
{
protected:
    uint32_t m_id;
    uint32_t m_levelCount;

public:
    explicit BaseTexture(uint32_t levelCount);
    ~BaseTexture() override;

    uint32_t getId() const;

    virtual DWORD SetLOD(DWORD LODNew) final;
    virtual DWORD GetLOD() final;
    virtual DWORD GetLevelCount() final;
    virtual HRESULT SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) final;
    virtual D3DTEXTUREFILTERTYPE GetAutoGenFilterType() final;
    virtual void GenerateMipSubLevels() final;

    virtual void FUN_48() final;
    virtual HRESULT FUN_4C(void* A1) final;
};
