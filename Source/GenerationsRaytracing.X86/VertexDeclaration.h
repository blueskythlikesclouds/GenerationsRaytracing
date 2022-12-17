#pragma once

#include "Unknown.h"

class Device;

class VertexDeclaration : public Unknown
{
public:
    std::vector<D3DVERTEXELEMENT9> vertexElements;

    VertexDeclaration(const D3DVERTEXELEMENT9* elements);

    size_t getVertexElementsSize() const
    {
        return vertexElements.size() * sizeof(D3DVERTEXELEMENT9);
    }

    virtual HRESULT GetDevice(Device** ppDevice) final;
    virtual HRESULT GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements) final;
};
