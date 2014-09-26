#ifndef THREADS_H_
#define THREADS_H_

////////////////////////////////////////////////////////////////////////////////
// macro DEFAULT_THREADING
// Selects the default threading model for certain components of Loki
// If you don't define it, it defaults to single-threaded
// All classes in Loki have configurable threading model; DEFAULT_THREADING
// affects only default template arguments
////////////////////////////////////////////////////////////////////////////////

// Last update: June 20, 2001

#ifndef DEFAULT_THREADING
#define DEFAULT_THREADING /**/ ::Loki::SingleThreaded
#endif

namespace Loki
{
////////////////////////////////////////////////////////////////////////////////
// class template SingleThreaded
// Implementation of the ThreadingModel policy used by various classes
// Implements a single-threaded model; no synchronization
////////////////////////////////////////////////////////////////////////////////

    template <class Host>
    class SingleThreaded
    {
    public:
        struct Lock
        {
            Lock() {}
            explicit Lock(const SingleThreaded&) {}
        };
        
        typedef Host VolatileType;

        typedef int IntType; 

        static IntType AtomicAdd(volatile IntType& lval, IntType val)
        { return lval += val; }
        
        static IntType AtomicSubtract(volatile IntType& lval, IntType val)
        { return lval -= val; }

        static IntType AtomicMultiply(volatile IntType& lval, IntType val)
        { return lval *= val; }
        
        static IntType AtomicDivide(volatile IntType& lval, IntType val)
        { return lval /= val; }
        
        static IntType AtomicIncrement(volatile IntType& lval)
        { return ++lval; }
        
        static IntType AtomicDecrement(volatile IntType& lval)
        { return --lval; }
        
        static void AtomicAssign(volatile IntType & lval, IntType val)
        { lval = val; }
        
        static void AtomicAssign(IntType & lval, volatile IntType & val)
        { lval = val; }
    };
    
#ifdef _WINDOWS_

////////////////////////////////////////////////////////////////////////////////
// class template ObjectLevelLockable
// Implementation of the ThreadingModel policy used by various classes
// Implements a object-level locking scheme
////////////////////////////////////////////////////////////////////////////////

    template <class Host>
    class ObjectLevelLockable
    {
        CRITICAL_SECTION mtx_;

    public:
        ObjectLevelLockable()
        {
            ::InitializeCriticalSection(&mtx_);
        }

        ~ObjectLevelLockable()
        {
            ::DeleteCriticalSection(&mtx_);
        }

        class Lock;
        friend class Lock;
        
        class Lock
        {
            ObjectLevelLockable& host_;
            
            Lock(const Lock&);
            Lock& operator=(const Lock&);
        public:

            explicit Lock(ObjectLevelLockable& host) : host_(host)
            {
                ::EnterCriticalSection(&host_.mtx_);
            }

            ~Lock()
            {
                ::LeaveCriticalSection(&host_.mtx_);
            }
        };

        typedef volatile Host VolatileType;

        typedef LONG IntType; 

        static IntType AtomicIncrement(volatile IntType& lval)
        { return InterlockedIncrement(&const_cast<IntType&>(lval)); }
        
        static IntType AtomicDecrement(volatile IntType& lval)
        { return InterlockedDecrement(&const_cast<IntType&>(lval)); }
        
        static void AtomicAssign(volatile IntType& lval, IntType val)
        { InterlockedExchange(&const_cast<IntType&>(lval), val); }
        
        static void AtomicAssign(IntType& lval, volatile IntType& val)
        { InterlockedExchange(&lval, val); }
    };
    
    template <class Host>
    class ClassLevelLockable
    {
        struct Initializer
        {   
            CRITICAL_SECTION mtx_;

            Initializer()
            {
                ::InitializeCriticalSection(&mtx_);
            }
            ~Initializer()
            {
                ::DeleteCriticalSection(&mtx_);
            }
        };
        
        static Initializer initializer_;

    public:
        class Lock;
        friend class Lock;
        
        class Lock
        {
            Lock(const Lock&);
            Lock& operator=(const Lock&);
        public:
            Lock()
            {
                ::EnterCriticalSection(&initializer_.mtx_);
            }
            explicit Lock(ClassLevelLockable&)
            {
                ::EnterCriticalSection(&initializer_.mtx_);
            }
            ~Lock()
            {
                ::LeaveCriticalSection(&initializer_.mtx_);
            }
        };

        typedef volatile Host VolatileType;

        typedef LONG IntType; 

        static IntType AtomicIncrement(volatile IntType& lval)
        { return InterlockedIncrement(&const_cast<IntType&>(lval)); }
        
        static IntType AtomicDecrement(volatile IntType& lval)
        { return InterlockedDecrement(&const_cast<IntType&>(lval)); }
        
        static void AtomicAssign(volatile IntType& lval, IntType val)
        { InterlockedExchange(&const_cast<IntType&>(lval), val); }
        
        static void AtomicAssign(IntType& lval, volatile IntType& val)
        { InterlockedExchange(&lval, val); }
    };
    
    template <class Host>
    typename ClassLevelLockable<Host>::Initializer 
    ClassLevelLockable<Host>::initializer_;
    
#endif    
}

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
////////////////////////////////////////////////////////////////////////////////

#endif
