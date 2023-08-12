#pragma once

struct VertexDeclaration
{
    std::unique_ptr<D3D12_INPUT_ELEMENT_DESC[]> inputElements;
    uint32_t inputElementsSize = 0;
    // 4 bytes wasted...
};