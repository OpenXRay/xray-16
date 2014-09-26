// Last update: June 20, 2001

#ifdef _MSC_VER
# pragma once
#else
# error "Visual C++ specific header !"
#endif

#ifndef VC_ALIGNMENT_INC_
#define VC_ALIGNMENT_INC_

#include "Typelist.h"
#include "static_check.h"

////////////////////////////////////////////////////////////////////////////////
// class template MaxAlign
// Computes the maximum alignof for all types in a typelist
// Usage: MaxAlign<TList>::result
////////////////////////////////////////////////////////////////////////////////

template <class TList> struct VC_MaxAlign;

template <> 
struct VC_MaxAlign< ::Loki::NullType >
{
    enum { result = 0 };
};

template <class TList>
struct VC_MaxAlign
{
private:
    ASSERT_TYPELIST(TList);

    typedef typename TList::Head Head;
    typedef typename TList::Tail Tail;

private:
    enum { headResult = __alignof(Head)        };
    enum { tailResult = VC_MaxAlign<Tail>::result };

public:
    enum { result = headResult > tailResult ? 
           headResult : tailResult };
};

                         
////////////////////////////////////////////////////////////////////////////////
// class VC_AlignedPODBase
// Defines a host of protected types used by VC_AlignedPOD (defined later)
// Could be just part of VC_AlignedPOD itself, but making it separate ought to 
// reduce compile times 
////////////////////////////////////////////////////////////////////////////////
class VC_AlignedPODBase
{
protected:

    template<unsigned AlignmentSize>
    struct AlignedPod
    {
        STATIC_CHECK(AlignmentSize == 0, BadAlignmentSize_OnlyUpTo128);
    };

//
// I used the macro because align(#) 
// only works with Integer literals
//
#define ALIGNED_POD(_size_)                                   \
template<> struct AlignedPod<_size_> {                        \
  __declspec(align(_size_)) struct type { char X[_size_]; };  \
  enum { alignment = __alignof(type) }; };                    \
STATIC_CHECK((_size_ == sizeof(AlignedPod<_size_>::type)), SizeofNotEqualSize); \
STATIC_CHECK((_size_ == (AlignedPod<_size_>::alignment)), SizeofNotEqualAlignof)


    ALIGNED_POD(1);
    ALIGNED_POD(2);
    ALIGNED_POD(4);
    ALIGNED_POD(8);
    ALIGNED_POD(16);
    ALIGNED_POD(32);
    ALIGNED_POD(64);
    ALIGNED_POD(128);
    // can be up to 8192 - is it realistic?

#undef ALIGNED_POD

};


////////////////////////////////////////////////////////////////////////////////
// class template VC_AlignedPOD
// Computes the alignment of all types in a typelist
////////////////////////////////////////////////////////////////////////////////

template <typename TList>
class VC_AlignedPOD : VC_AlignedPODBase
{
    enum { maxAlign = VC_MaxAlign<TList>::result };

public:
    typedef typename AlignedPod<maxAlign>::type Result;
};

#endif // VC_ALIGNMENT_INC_

