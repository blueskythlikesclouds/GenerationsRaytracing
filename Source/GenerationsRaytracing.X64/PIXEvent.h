#pragma once

#define PIX_EVENT_CONCAT(left, right) PIXEvent left##right(getUnderlyingGraphicsCommandList(), __FUNCTION__)
#define PIX_EVENT() PIX_EVENT_CONCAT(pixEvent, __LINE__)

class PIXEvent
{
protected:
    ID3D12GraphicsCommandList* m_commandList;

public:
    PIXEvent(ID3D12GraphicsCommandList* commandList, const char* formatString);
    ~PIXEvent();
};
