#pragma once

#ifdef _DEBUG

#define THROW_IF_FAILED(x) \
    do \
    { \
        const NVSDK_NGX_Result result = x; \
        if (NVSDK_NGX_FAILED(result)) \
        { \
            OutputDebugStringW(GetNGXResultAsString(result)); \
            assert(!#x); \
        } \
    } while (0)

#else
#define THROW_IF_FAILED(x) x
#endif

class Device;

class NGX
{
protected:
    ComPtr<ID3D12Device> m_device;
    NVSDK_NGX_Parameter* m_parameters = nullptr;

public:
    NGX(const Device& device);
    ~NGX();

    NVSDK_NGX_Parameter* getParameters() const;

    bool valid() const;
};
