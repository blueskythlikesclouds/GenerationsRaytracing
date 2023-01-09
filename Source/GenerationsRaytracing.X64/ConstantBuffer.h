#pragma once

#define VS_UNUSED_CONSTANT   196
#define VS_BOOLEANS          0

#define PS_UNUSED_CONSTANT   148
#define PS_ENABLE_ALPHA_TEST 0
#define PS_ALPHA_THRESHOLD   1
#define PS_BOOLEANS          2

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