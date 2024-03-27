
#ifndef _REFERENCE_COUNTED_
#define _REFERENCE_COUNTED_

// Safe memory freeing
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)                                                                                                 \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            delete (p);                                                                                                \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)                                                                                           \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            delete[] (p);                                                                                              \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)                                                                                                \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            (p)->Release();                                                                                            \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#endif

#ifndef SAFE_RELEASE_FORCE
#define SAFE_RELEASE_FORCE(p)                                                                                          \
    {                                                                                                                  \
        if (p)                                                                                                         \
        {                                                                                                              \
            (p)->ReleaseForce();                                                                                       \
            (p) = NULL;                                                                                                \
        }                                                                                                              \
    }
#endif

class ReferenceCounted
{
    int m_RefCount;

public:
    ReferenceCounted() : m_RefCount(0) {}

    ReferenceCounted(ReferenceCounted&& r) : m_RefCount(std::move(r.m_RefCount)) {}

    ReferenceCounted& operator=(ReferenceCounted&& r)
    {
        m_RefCount = std::move(r.m_RefCount);
        return *this;
    }

    void AddRef() { InterlockedIncrement((volatile LONG*)&m_RefCount); }
    void Release()
    {
        if (!InterlockedDecrement((volatile LONG*)&m_RefCount))
        {
            delete this;
        }
    }

protected:
    virtual ~ReferenceCounted() {}
};


#endif
