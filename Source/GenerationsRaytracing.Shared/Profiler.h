#pragma once

//#define ENABLE_PROFILER

#ifndef ENABLE_PROFILER
#define PROFILER(name) ((void)0)
#define PUSH_PROFILER(name) ((void)0)
#define POP_PROFILER() ((void)0)

#else
#include <chrono>
#include <stack>
#include <Windows.h>

#define PROFILER_CONCAT_1(name, line) Profiler profiler##line(name)
#define PROFILER_CONCAT_0(name, line) PROFILER_CONCAT_1(name, line)
#define PROFILER(name) PROFILER_CONCAT_0(name, __LINE__)

struct Profiler
{
    const char* name;
    std::chrono::system_clock::time_point time;

    Profiler(const char* name)
        : name(name)
        , time(std::chrono::system_clock::now())
    {
    }

    Profiler(const Profiler&) = delete;

    Profiler(Profiler&& profiler)
        : name(profiler.name)
        , time(profiler.time)
    {
        profiler.name = nullptr;
    }

    ~Profiler()
    {
        if (name == nullptr)
            return;

        char text[1024];

        sprintf(text, "%s: %g ms\n",
            name,
            std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::system_clock::now() - time).count());

        OutputDebugStringA(text);
    }
};

inline thread_local std::stack<Profiler> PROFILER_STACK;

#define PUSH_PROFILER(name) PROFILER_STACK.push({ name })
#define POP_PROFILER() PROFILER_STACK.pop()

#endif
