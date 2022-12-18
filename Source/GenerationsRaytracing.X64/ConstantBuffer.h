#pragma once

template<int N>
struct ConstantBuffer
{
    FLOAT c[N][4];

    void writeConstant(size_t startRegister, float* data, size_t count)
    {
        memcpy(c[startRegister], data, count * sizeof(FLOAT[4]));
    }

    void writeBuffer(nvrhi::ICommandList* commandList, nvrhi::IBuffer* constantBuffer)
    {
        commandList->writeBuffer(constantBuffer, c, sizeof(c));
    }
};

#define SHARED_FLAGS_ENABLE_ALPHA_TEST (1 << 0)
#define SHARED_FLAGS_HAS_10_BIT_NORMAL (1 << 1)
#define SHARED_FLAGS_HAS_BINORMAL      (1 << 2)

struct SharedConstantBuffer
{
    uint32_t booleans = 0;
    uint32_t flags = 0;
    float alphaThreshold = 0.5f;
};