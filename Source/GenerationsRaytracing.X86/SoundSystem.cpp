#include "SoundSystem.h"

void __fastcall setSoundSystemCriticalSection(boost::shared_ptr<Hedgehog::Base::CCriticalSectionD3D9>* criticalSection, void*, void*)
{
    *criticalSection = *reinterpret_cast<Hedgehog::Base::CSynchronizedObject*>(0x1E77270)->m_pCriticalSection;
}

void SoundSystem::init()
{
    // Sound system mutexes can deadlock each other due to
    // incorrect order. We can fix it by making them the same mutex.
    WRITE_NOP(0x760DBA, 5);
    WRITE_MEMORY(0x760DC6, uint8_t, 0xEB);
    WRITE_CALL(0x760DD9, setSoundSystemCriticalSection);
}
