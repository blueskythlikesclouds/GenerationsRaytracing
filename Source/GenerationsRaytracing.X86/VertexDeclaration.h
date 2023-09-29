#pragma once

#include "Unknown.h"

class Device;

class VertexDeclaration : public Unknown
{
protected:
    uint32_t m_id;
    std::unique_ptr<D3DVERTEXELEMENT9[]> m_vertexElements;
    uint32_t m_vertexElementsSize;

public:
    explicit VertexDeclaration(const D3DVERTEXELEMENT9* vertexElements);
    ~VertexDeclaration() override;

    uint32_t getId() const;

    const D3DVERTEXELEMENT9* getVertexElements() const;
    uint32_t getVertexElementsSize() const;

    virtual HRESULT GetDevice(Device** ppDevice) final;
    virtual HRESULT GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements) final;
};
