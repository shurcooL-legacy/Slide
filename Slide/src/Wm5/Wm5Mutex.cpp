// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 5.0.0 (2010/01/01)

#include "Wm5CorePCH.h"
#include "Wm5Mutex.h"
#include "Wm5Assert.h"
using namespace Wm5;

#if defined(WIN32)
//----------------------------------------------------------------------------
#include <windows.h>
//----------------------------------------------------------------------------
Mutex::Mutex ()
{
    mMutex = CreateMutex(NULL, FALSE, NULL);
    assertion(mMutex != 0, "Failed to create mutex\n");
}
//----------------------------------------------------------------------------
Mutex::~Mutex ()
{
    BOOL closed = CloseHandle((HANDLE)mMutex);
    assertion(closed == TRUE, "Failed to destroy mutex\n");
    WM5_UNUSED(closed);
}
//----------------------------------------------------------------------------
void Mutex::Enter ()
{
    DWORD result = WaitForSingleObject((HANDLE)mMutex, INFINITE);
    WM5_UNUSED(result);
    // result:
    //   WAIT_ABANDONED (0x00000080)
    //   WAIT_OBJECT_0  (0x00000000), signaled
    //   WAIT_TIMEOUT   (0x00000102), [not possible with INFINITE]
    //   WAIT_FAILED    (0xFFFFFFFF), not signaled
}
//----------------------------------------------------------------------------
void Mutex::Leave ()
{
    BOOL released = ReleaseMutex((HANDLE)mMutex);
    WM5_UNUSED(released);
}
//----------------------------------------------------------------------------
#elif defined(__LINUX__) || defined(__APPLE__)
//----------------------------------------------------------------------------
Mutex::Mutex ()
{
    int result;
    WM5_UNUSED(result);
    
    result = pthread_mutexattr_init(&mMutex.Attribute);
    // successful = 0
    // errors = ENOMEM

    result = pthread_mutexattr_settype(&mMutex.Attribute,
         PTHREAD_MUTEX_RECURSIVE);
    // successful = 0
    
    result = pthread_mutex_init(&mMutex.Mutex, &mMutex.Attribute);
    // successful = 0
    // errors = EAGAIN, ENOMEM, EPERM, EBUSY, EINVAL
}
//----------------------------------------------------------------------------
Mutex::~Mutex ()
{
    int result;
    WM5_UNUSED(result);

    result = pthread_mutex_destroy(&mMutex.Mutex);
    // successful = 0
    // errors = EINVAL

    result = pthread_mutexattr_destroy(&mMutex.Attribute);
    // successful = 0
    // errors = EBUSY, EINVAL
}
//----------------------------------------------------------------------------
void Mutex::Enter ()
{
    int result = pthread_mutex_lock(&mMutex.Mutex);
    WM5_UNUSED(result);
    // successful = 0
    // errors = EINVAL, EDEADLK
}
//----------------------------------------------------------------------------
void Mutex::Leave ()
{
    int result = pthread_mutex_unlock(&mMutex.Mutex);
    WM5_UNUSED(result);
    // successful = 0
    // errors = EINVAL, EPERM
}
//----------------------------------------------------------------------------
#else
#error Other platforms not yet implemented.
#endif
//----------------------------------------------------------------------------
