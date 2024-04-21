#pragma once

constexpr u32 occq_size_base = 768; // // queue for occlusion queries
constexpr u32 occq_size = 2 * occq_size_base * R__NUM_PARALLEL_CONTEXTS; // // queue for occlusion queries

// must conform to following order of allocation/free
// a(A), a(B), a(C), a(D), ....
// f(A), f(B), f(C), f(D), ....
// a(A), a(B), a(C), a(D), ....
//	this mean:
//		use as litle of queries as possible
//		first try to use queries allocated first
//	assumption:
//		used queries number is much smaller than total count

class R_occlusion
{
private:
    struct Query
    {
#if defined(USE_DX9) || defined(USE_DX11)
        ID3DQuery* Q;
#elif defined(USE_OGL)
        GLuint Q;
#else
#   error No graphics API selected or enabled!
#endif
        u32 order;
    };

    static constexpr u32 iInvalidHandle = 0xFFFFFFFF;

    bool enabled;
    xr_vector<Query> pool; // sorted (max ... min), insertions are usually at the end
    xr_vector<Query> used; // id's are generated from this and it is cleared from back only
    xr_vector<u32> fids; // free id's

    Lock render_lock{};
public:
#if defined(USE_DX11)
    typedef u64 occq_result;
#elif defined(USE_DX9) || defined(USE_OGL)
    typedef u32 occq_result;
#else
#   error No graphics API selected or enabled!
#endif
public:
    R_occlusion();
    ~R_occlusion();

    void occq_create(u32 limit);
    void occq_destroy();
    u32 occq_begin(u32& ID); // returns 'order'
    void occq_end(u32& ID);
    occq_result occq_get(u32& ID);
};
