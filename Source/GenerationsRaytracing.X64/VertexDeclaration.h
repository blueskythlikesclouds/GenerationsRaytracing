#pragma once

struct VertexDeclaration
{
    std::unique_ptr<D3D12_INPUT_ELEMENT_DESC[]> inputElements;
    uint32_t inputElementsSize = 0;
    bool isFVF = false;
    bool enable10BitNormal = false;
    bool enableBlendWeight = false;
    bool enableBlendIndices = false;
};