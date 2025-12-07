#include "ShaderCache.h"

#include "Message.h"
#include "MessageSender.h"

HOOK(void, __fastcall, GameplayFlowStageEnter, 0xD05530, void* This)
{
    s_messageSender.oneShotMessage<MsgSaveShaderCache>();

    originalGameplayFlowStageEnter(This);
}

HOOK(void, __fastcall, GameplayFlowTitleEnter, 0xCF9750, void* This)
{
    s_messageSender.oneShotMessage<MsgSaveShaderCache>();

    originalGameplayFlowTitleEnter(This);
}

void ShaderCache::init()
{
    INSTALL_HOOK(GameplayFlowStageEnter);
    INSTALL_HOOK(GameplayFlowTitleEnter);
}
