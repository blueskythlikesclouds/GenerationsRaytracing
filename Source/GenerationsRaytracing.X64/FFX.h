#pragma once

#define FFX_THROW_IF_FAILED(x) \
    do \
    { \
        ffx::ReturnCode returnCode = x; \
        if (returnCode != ffx::ReturnCode::Ok) \
        { \
            assert(0 && #x); \
        } \
    } while (0)

template<typename T>
static T makeFfxObject()
{
    alignas(alignof(T)) uint8_t bytes[sizeof(T)]{};
    new (bytes) T();
    return *reinterpret_cast<T*>(bytes);
}