#include "VertexDeclaration.h"

#include "Message.h"
#include "MessageSender.h"

static std::atomic<uint32_t> s_curId = 0;

VertexDeclaration::VertexDeclaration(const D3DVERTEXELEMENT9* vertexElements) : m_id(++s_curId)
{
    m_vertexElementsSize = 0;

    while (true)
    {
        auto& vertexElement = vertexElements[m_vertexElementsSize];
        ++m_vertexElementsSize;

        if (vertexElement.Stream == 0xFF || vertexElement.Type == D3DDECLTYPE_UNUSED)
            break;
    }

    m_vertexElements = std::make_unique<D3DVERTEXELEMENT9[]>(m_vertexElementsSize);
    memcpy(m_vertexElements.get(), vertexElements, sizeof(D3DVERTEXELEMENT9) * m_vertexElementsSize);
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
    memcpy(pElement, m_vertexElements.get(), sizeof(D3DVERTEXELEMENT9) * m_vertexElementsSize);
    *pNumElements = m_vertexElementsSize;
    return S_OK;
}
