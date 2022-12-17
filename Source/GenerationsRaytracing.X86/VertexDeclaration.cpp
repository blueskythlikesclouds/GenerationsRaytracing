#include "VertexDeclaration.h"

VertexDeclaration::VertexDeclaration(const D3DVERTEXELEMENT9* elements)
{
    for (int i = 0; ; i++)
    {
        vertexElements.push_back(elements[i]);

        if (elements[i].Stream == 0xFF)
            break;
    }

    vertexElements.shrink_to_fit();
}

FUNCTION_STUB(HRESULT, VertexDeclaration::GetDevice, Device** ppDevice)

HRESULT VertexDeclaration::GetDeclaration(D3DVERTEXELEMENT9* pElement, UINT* pNumElements)
{
    memcpy(pElement, vertexElements.data(), getVertexElementsSize());
    *pNumElements = vertexElements.size();

    return S_OK;
}