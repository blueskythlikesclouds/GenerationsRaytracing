#include <cassert>

inline Event::Event(LPCTSTR name, BOOL initialState)
{
    m_handle = CreateEvent(
        nullptr,
        TRUE,
        initialState,
        name);

    assert(m_handle != nullptr);
}

inline Event::Event(LPCTSTR name)
{
    m_handle = OpenEvent(
        EVENT_ALL_ACCESS,
        FALSE,
        name);

    assert(m_handle != nullptr);
}

inline void Event::wait() const
{
    const DWORD result = WaitForSingleObject(m_handle, INFINITE);
    assert(result == WAIT_OBJECT_0);
}

inline bool Event::waitImm() const
{
    const DWORD result = WaitForSingleObject(m_handle, 1u);
    return result == WAIT_OBJECT_0;
}

inline void Event::set() const
{
    const BOOL result = SetEvent(m_handle);
    assert(result == TRUE);
}

inline void Event::reset() const
{
    const BOOL result = ResetEvent(m_handle);
    assert(result == TRUE);
}
