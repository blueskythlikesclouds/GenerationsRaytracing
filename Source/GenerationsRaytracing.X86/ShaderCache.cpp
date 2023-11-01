#include "ShaderCache.h"

#include "Message.h"
#include "MessageSender.h"

HOOK(void, __fastcall, GameplayFlowStageEnter, 0xD05530, void* This)
{
    s_messageSender.makeMessage<MsgSaveShaderCache>();
    s_messageSender.endMessage();

    originalGameplayFlowStageEnter(This);
}

HOOK(void, __fastcall, GameplayFlowTitleEnter, 0xCF9750, void* This)
{
    s_messageSender.makeMessage<MsgSaveShaderCache>();
    s_messageSender.endMessage();

    originalGameplayFlowTitleEnter(This);
}

void ShaderCache::init()
{
    INSTALL_HOOK(GameplayFlowStageEnter);
    INSTALL_HOOK(GameplayFlowTitleEnter);
}
