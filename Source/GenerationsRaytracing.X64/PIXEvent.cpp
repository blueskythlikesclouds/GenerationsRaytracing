#include "PIXEvent.h"

PIXEvent::PIXEvent(ID3D12GraphicsCommandList* commandList, const char* formatString)
    : m_commandList(commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, formatString);
}

PIXEvent::~PIXEvent()
{
    PIXEndEvent(m_commandList);
}
