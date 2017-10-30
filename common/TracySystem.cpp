#ifdef _WIN32
#  include <windows.h>
#else
#  include <pthread.h>
#  include <string.h>
#  include <unistd.h>
#endif

#ifdef __ANDROID__
#  include <sys/prctl.h>
#endif

#include <inttypes.h>

#include "TracySystem.hpp"

namespace tracy
{

void SetThreadName( std::thread& thread, const char* name )
{
    SetThreadName( thread.native_handle(), name );
}

void SetThreadName( std::thread::native_handle_type handle, const char* name )
{
#ifdef _WIN32
#  ifdef NTDDI_WIN10_RS2
    wchar_t buf[256];
    mbstowcs( buf, name, 256 );
    SetThreadDescription( static_cast<HANDLE>( handle ), buf );
#  else
    const DWORD MS_VC_EXCEPTION=0x406D1388;
#    pragma pack( push, 8 )
    struct THREADNAME_INFO
    {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };
#    pragma pack(pop)

    DWORD ThreadId = GetThreadId( static_cast<HANDLE>( handle ) );
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = ThreadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
#  endif
#elif defined _GNU_SOURCE
    const auto sz = strlen( name );
    if( sz <= 15 )
    {
        pthread_setname_np( handle, name );
    }
    else
    {
        char buf[16];
        memcpy( buf, name, 15 );
        buf[15] = '\0';
        pthread_setname_np( handle, buf );
    }
#endif
}

const char* GetThreadName( uint64_t id )
{
    static char buf[256];
#ifdef _WIN32
#  ifdef NTDDI_WIN10_RS2
    auto hnd = OpenThread( THREAD_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)id );
    if( hnd != 0 )
    {
        PWSTR tmp;
        GetThreadDescription( hnd, &tmp );
        auto ret = wcstombs( buf, tmp, 256 );
        CloseHandle( hnd );
        if( ret != 0 )
        {
            return buf;
        }
    }
#  endif
#elif defined __ANDROID__
    if( prctl( PR_GET_NAME, (unsigned long)buf, 0, 0, 0 ) == 0 )
    {
        return buf;
    }
#elif defined _GNU_SOURCE
    if( pthread_getname_np( (pthread_t)id, buf, 256 ) == 0 )
    {
        return buf;
    }
#endif
    sprintf( buf, "%" PRIu64, id );
    return buf;
}

}
