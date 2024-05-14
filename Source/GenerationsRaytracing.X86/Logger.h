#pragma once

enum class LogType
{
    Success,
    Normal,
    Warning,
    Error
};

class Logger
{
public:
    static void log(LogType logType, const char* text);
    static void logFormatted(LogType logType, const char* format, ...);

    static void renderImgui();
};
