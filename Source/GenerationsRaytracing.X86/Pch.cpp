#include "Pch.h"

#ifndef _DEBUG
void __cdecl boost::throw_exception(class stdext::exception const&)
{
    __debugbreak();
}
#endif