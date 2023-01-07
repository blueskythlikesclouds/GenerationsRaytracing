#include "Pch.h"

#if !_DEBUG

namespace boost
{
    void throw_exception(std::exception const& e)
    {
        __debugbreak();
    }
}

#endif