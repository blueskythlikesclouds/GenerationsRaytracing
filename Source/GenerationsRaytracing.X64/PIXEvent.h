#pragma once

#define PIX_EVENT_CONCAT(LEFT, RIGHT) PIXEvent LEFT##RIGHT(getUnderlyingGraphicsCommandList(), __FUNCTION__ "()")
#define PIX_EVENT() PIX_EVENT_CONCAT(pixEvent, __LINE__)

#define PIX_BEGIN_EVENT(FORMAT_STRING) PIXBeginEvent(getUnderlyingGraphicsCommandList(), PIX_COLOR_DEFAULT, __FUNCTION__ "(): " FORMAT_STRING)
#define PIX_END_EVENT() PIXEndEvent(getUnderlyingGraphicsCommandList())

class PIXEvent
{
protected:
    ID3D12GraphicsCommandList* m_commandList;

public:
    PIXEvent(ID3D12GraphicsCommandList* commandList, const char* formatString);
    ~PIXEvent();
};
