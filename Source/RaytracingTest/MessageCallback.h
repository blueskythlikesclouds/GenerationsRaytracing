#pragma once

class MessageCallback : public nvrhi::IMessageCallback
{
public:
    void message(nvrhi::MessageSeverity severity, const char* messageText) override
    {
        constexpr const char* SEVERITY_TEXT[] =
        {
            "Info",
            "Warning",
            "Error",
            "Fatal"
        };

        printf("%s: %s\n", SEVERITY_TEXT[(int)severity], messageText);
        assert(severity != nvrhi::MessageSeverity::Error && severity != nvrhi::MessageSeverity::Fatal);
    }
};
