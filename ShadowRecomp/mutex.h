#pragma once

#ifdef _WIN32

struct Mutex : CRITICAL_SECTION
{
    Mutex()
    {
        printf("[MUTEX] Initializing %p\n", this);
        InitializeCriticalSection(this);
    }
    ~Mutex()
    {
        DeleteCriticalSection(this);
    }

    void lock()
    {
        EnterCriticalSection(this);
    }

    void unlock()
    {
        LeaveCriticalSection(this);
    }
};

#else

using Mutex = std::mutex;

#endif
