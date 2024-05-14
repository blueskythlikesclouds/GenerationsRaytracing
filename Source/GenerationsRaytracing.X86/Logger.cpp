#include "Logger.h"
#include "Mutex.h"
#include "LockGuard.h"

static Mutex s_mutex;
static std::vector<std::pair<LogType, std::string>> s_logs;
static size_t s_previousSize;

void Logger::log(LogType logType, const char* text)
{
    LockGuard lock(s_mutex);
    s_logs.emplace_back(logType, text);
}

void Logger::logFormatted(LogType logType, const char* format, ...)
{
    char text[0x400];

    va_list args;
    va_start(args, format);
    vsprintf_s(text, format, args);
    va_end(args);

    log(logType, text);
}

static constexpr ImVec4 s_colors[] =
{
    ImVec4(0.13f, 0.7f, 0.3f, 1.0f), // Success
    ImVec4(1.0f, 1.0f, 1.0f, 1.0f), // Normal
    ImVec4(1.0f, 0.78f, 0.054f, 1.0f), // Warning
    ImVec4(1.0f, 0.02f, 0.02f, 1.0f) // Error
};

void Logger::renderImgui()
{
    if (ImGui::BeginListBox("##Logs", {-FLT_MIN, -FLT_MIN}))
    {
        ImGui::PushTextWrapPos();

        LockGuard lock(s_mutex);

        for (auto& [logType, log] : s_logs)
            ImGui::TextColored(s_colors[static_cast<size_t>(logType)], log.c_str());

        ImGui::PopTextWrapPos();

        if (s_previousSize != s_logs.size())
            ImGui::SetScrollHereY();

        s_previousSize = s_logs.size();

        ImGui::EndListBox();
    }
}
