#pragma once

template<typename T>
T alignUp(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

template<typename T>
T alignDown(T value, T alignment)
{
    return value & ~(alignment - 1);
}