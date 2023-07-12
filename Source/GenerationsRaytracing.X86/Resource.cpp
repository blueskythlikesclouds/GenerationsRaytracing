#include "Resource.h"

FUNCTION_STUB(HRESULT, E_NOTIMPL, Resource::GetDevice, Device** ppDevice)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Resource::SetPrivateData, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Resource::GetPrivateData, REFGUID refguid, void* pData, DWORD* pSizeOfData)

FUNCTION_STUB(HRESULT, E_NOTIMPL, Resource::FreePrivateData, REFGUID refguid)

FUNCTION_STUB(DWORD, E_NOTIMPL, Resource::SetPriority, DWORD PriorityNew)

FUNCTION_STUB(DWORD, NULL, Resource::GetPriority)

FUNCTION_STUB(void, , Resource::PreLoad)

FUNCTION_STUB(D3DRESOURCETYPE, static_cast<D3DRESOURCETYPE>(NULL), Resource::GetType)