#pragma once

#if defined(__ANDROID__)
#   include <pthread.h>

int xrGameSpyAndroidCancel(pthread_t thread);
int xrGameSpyAndroidDetach(pthread_t thread);

#   define pthread_cancel xrGameSpyAndroidCancel
#   define pthread_detach xrGameSpyAndroidDetach
#endif
