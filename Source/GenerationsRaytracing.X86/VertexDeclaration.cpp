#include "VertexDeclaration.h"

VertexDeclaration::VertexDeclaration(const D3DVERTEXELEMENT9* elements)
{
    for (int i = 0; ; i++)
    {
        if (elements[i].Stream == 0xFF || elements[i].Type == D3DDECLTYPE_UNUSED)
            break;

        vertexElements.push_back(elements[i]);
    }

    vertexElements.shrink_to_fit();
}

FUNCTION_STUB(HRESULT, VertexDeclaration::GetDevice, Device** ppDevice)

HRESULT VertexDeclaration::GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements)
{
    memcpy(pElement, vertexElements.data(), getVertexElementsSize());
    pElement[vertexElements.size()] = D3DDECL_END();
    *pNumElements = vertexElements.size() + 1;

    return S_OK;
}