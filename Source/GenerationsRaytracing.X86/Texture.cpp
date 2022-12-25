#include "Texture.h"

#include "Message.h"
#include "MessageSender.h"
#include "Surface.h"

Texture::Texture() = default;

Texture::Texture(size_t width, size_t height) : width(width), height(height)
{
}

Texture::~Texture() = default;

HRESULT Texture::GetLevelDesc(UINT Level, D3DSURFACE_DESC *pDesc)
{
    pDesc->Width = width >> Level;
    pDesc->Height = height >> Level;

    return S_OK;
}

HRESULT Texture::GetSurfaceLevel(UINT Level, Surface** ppSurfaceLevel)
{
    if (!surface)
        surface.Attach(new Surface(this));

    surface.CopyTo(ppSurfaceLevel);
    return S_OK;
}

HRESULT Texture::LockRect(UINT Level, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags)
{
    const size_t size = width * height * 4;
    const auto msg = msgSender.start<MsgWriteTexture>(size);

    msg->texture = id;
    msg->size = size;
    msg->pitch = width * 4;

    pLockedRect->Pitch = (INT)msg->pitch;
    pLockedRect->pBits = MSG_DATA_PTR(msg);

    return S_OK;
}

HRESULT Texture::UnlockRect(UINT Level)
{
    msgSender.finish();
    return S_OK;
}

FUNCTION_STUB(HRESULT, Texture::AddDirtyRect, CONST RECT* pDirtyRect)

HRESULT Texture::BeginSfdDecodeCallback(uintptr_t, Texture*& texture, uintptr_t, uintptr_t)
{
    texture = this;
    return S_OK;
}

HRESULT Texture::EndSfdDecodeCallback(uintptr_t)
{
    return S_OK;
}