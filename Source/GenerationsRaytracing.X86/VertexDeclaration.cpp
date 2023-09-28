#include "VertexDeclaration.h"

#include "FreeListAllocator.h"

static FreeListAllocator s_freeListAllocator;

VertexDeclaration::VertexDeclaration(const D3DVERTEXELEMENT9* vertexElements)
{
    m_id = s_freeListAllocator.allocate();

    m_vertexElementsSize = 0;

    while (true)
    {
        ++m_vertexElementsSize;

        if (vertexElements[m_vertexElementsSize - 1].Type == D3DDECLTYPE_UNUSED)
            break;
    }

    m_vertexElements = std::make_unique<D3DVERTEXELEMENT9[]>(m_vertexElementsSize);
    memcpy(m_vertexElements.get(), vertexElements, sizeof(D3DVERTEXELEMENT9) * m_vertexElementsSize);
}

VertexDeclaration::VertexDeclaration(DWORD fvf)
{
    m_id = s_freeListAllocator.allocate();

    m_vertexElementsSize = 0;

    // TODO
}

VertexDeclaration::~VertexDeclaration()
{
    s_freeListAllocator.free(m_id);
}

uint32_t VertexDeclaration::getId() const
{
    return m_id;
}

const D3DVERTEXELEMENT9* VertexDeclaration::getVertexElements() const
{
    return m_vertexElements.get();
}

uint32_t VertexDeclaration::getVertexElementsSize() const
{
    return m_vertexElementsSize;
}

FUNCTION_STUB(HRESULT, E_NOTIMPL, VertexDeclaration::GetDevice, Device** ppDevice)

HRESULT VertexDeclaration::GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements)
{
    memcpy(pElement, m_vertexElements.get(), m_vertexElementsSize);
    *pNumElements = m_vertexElementsSize;
    return S_OK;
}
