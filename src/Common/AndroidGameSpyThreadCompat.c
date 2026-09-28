#include "AndroidGameSpyThreadCompat.h"

#if defined(__ANDROID__)
#include <errno.h>

#undef pthread_cancel
#undef pthread_detach

enum { XR_GAMESPY_MAX_PENDING_CANCELLATIONS = 64 };

static pthread_mutex_t xrGameSpyCancellationLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t xrGameSpyPendingCancellations[XR_GAMESPY_MAX_PENDING_CANCELLATIONS];
static unsigned char xrGameSpyCancellationUsed[XR_GAMESPY_MAX_PENDING_CANCELLATIONS];

static int xrGameSpyFindCancellation(pthread_t thread)
{
    int index;
    for (index = 0; index < XR_GAMESPY_MAX_PENDING_CANCELLATIONS; ++index)
    {
        if (xrGameSpyCancellationUsed[index] && pthread_equal(xrGameSpyPendingCancellations[index], thread))
            return index;
    }
    return -1;
}

int xrGameSpyAndroidCancel(pthread_t thread)
{
    // Bionic deliberately omits pthread_cancel. GameSpy uses cancellation
    // only for its asynchronous DNS worker and frees the worker argument as
    // soon as this function returns. Mark the thread before joining it so its
    // self-detach path becomes a no-op, then wait until getaddrinfo and every
    // access to that argument have completed. This can wait for DNS timeout,
    // but it preserves memory safety and the asynchronous resolver itself.
    int slot = -1;
    pthread_mutex_lock(&xrGameSpyCancellationLock);
    slot = xrGameSpyFindCancellation(thread);
    if (slot < 0)
    {
        int index;
        for (index = 0; index < XR_GAMESPY_MAX_PENDING_CANCELLATIONS; ++index)
        {
            if (!xrGameSpyCancellationUsed[index])
            {
                slot = index;
                xrGameSpyPendingCancellations[index] = thread;
                xrGameSpyCancellationUsed[index] = 1;
                break;
            }
        }
    }
    pthread_mutex_unlock(&xrGameSpyCancellationLock);

    const int result = pthread_join(thread, NULL);

    if (slot >= 0)
    {
        pthread_mutex_lock(&xrGameSpyCancellationLock);
        xrGameSpyCancellationUsed[slot] = 0;
        pthread_mutex_unlock(&xrGameSpyCancellationLock);
    }

    // If the worker reached its detach call immediately before it was marked,
    // all accesses to the caller-owned handle had already finished. Treat the
    // resulting non-joinable-thread status as a completed cancellation.
    return result == 0 || result == EINVAL || result == ESRCH ? 0 : result;
}

int xrGameSpyAndroidDetach(pthread_t thread)
{
    pthread_mutex_lock(&xrGameSpyCancellationLock);
    const int cancellation = xrGameSpyFindCancellation(thread);
    pthread_mutex_unlock(&xrGameSpyCancellationLock);

    return cancellation >= 0 ? 0 : pthread_detach(thread);
}
#endif
