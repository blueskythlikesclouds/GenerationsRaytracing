inline Mutex::Mutex()
{
    InitializeCriticalSection(this);
}

inline Mutex::~Mutex()
{
    DeleteCriticalSection(this);
}

inline void Mutex::lock()
{
    EnterCriticalSection(this);
}

inline void Mutex::unlock()
{
    LeaveCriticalSection(this);
}


