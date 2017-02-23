/* Basic platform-independent macro definitions for mutexes,
   thread-specific data and parameters for malloc.
   Copyright (C) 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _WIN32_MALLOC_MACHINE_H
#define _WIN32_MALLOC_MACHINE_H

#include "win32.c"

#if 1
typedef volatile long mutex_t;

#define MUTEX_INITIALIZER          0
#define mutex_init(m)              (*(m) = 0)
#define mutex_lock(m)              slwait(m)
#define mutex_trylock(m)           sltrywait(m)
#define mutex_unlock(m)            slrelease(m)
#else
/* This won't work on Windows 9x. Can't say I personally care (crappy OS) */
typedef CRITICAL_SECTION mutex_t;

#define MUTEX_INITIALIZER          { 0 }
#define mutex_init(m)              (!InitializeCriticalSectionAndSpinCount(m, 4000))
#define mutex_lock(m)              (EnterCriticalSection(m), 0)
#define mutex_trylock(m)           (!TryEnterCriticalSection(m))
#define mutex_unlock(m)            (LeaveCriticalSection(m), 0)
#endif

typedef DWORD tsd_key_t;
#define tsd_key_create(key, destr) (*(key)=TlsAlloc(), TLS_OUT_OF_INDEXES!=(*key))
#define tsd_setspecific(key, data) (!TlsSetValue(key, data))
#define tsd_getspecific(key, vptr) (vptr = TlsGetValue(key))

#define thread_atfork(prepare, parent, child) do {} while(0)


#ifndef atomic_full_barrier
# if defined(__GNUC__)
#  define atomic_full_barrier() __asm ("" ::: "memory")
# else
#  define atomic_full_barrier()
# endif
#endif

#ifndef atomic_read_barrier
# define atomic_read_barrier() atomic_full_barrier ()
#endif

#ifndef atomic_write_barrier
# define atomic_write_barrier() atomic_full_barrier ()
#endif

#ifndef DEFAULT_TOP_PAD
# define DEFAULT_TOP_PAD 131072
#endif

#endif /* !defined(_WIN32_MALLOC_MACHINE_H) */
