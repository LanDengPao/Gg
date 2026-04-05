#include "LambdaThread.h"

// Default Qt doesn't do this in release Qt builds, which is all we use
#if defined(Q_OS_WIN32)

#include <windows.h>

typedef HRESULT(WINAPI* PFN_SetThreadDescription)(HANDLE hThread, PCWSTR lpThreadDescription);

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;        // Must be 0x1000.
    LPCSTR szName;       // Pointer to name (in user addr space).
    DWORD dwThreadID;    // Thread ID (-1=caller thread).
    DWORD dwFlags;       // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

static void SetThreadNameWithException(const char* name)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;
    __try
    {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)(&info));
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}

void LambdaThread::windowsSetName()
{
    // try to use the fancy modern API
    static PFN_SetThreadDescription setThreadDesc = (PFN_SetThreadDescription)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "SetThreadDescription");

    if (setThreadDesc)
    {
        setThreadDesc(GetCurrentThread(), m_Name.toStdWString().c_str());
    }
    else
    {
        // don't throw the exception if there's no debugger present
        if (!IsDebuggerPresent())
            return;

        SetThreadNameWithException(m_Name.toStdString().c_str());
    }
}

#else

void LambdaThread::windowsSetName()
{
}

#endif