#pragma once
#include "xr_vector.h"

template <typename _Ty, class _C = xr_vector<_Ty>>
class xr_stack
{
public:
    typedef typename _C::allocator_type allocator_type;
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::size_type size_type;
    typedef xr_stack<_Ty, _C> _Myt;

    allocator_type get_allocator() const { return c.get_allocator(); }
    bool empty() const { return c.empty(); }
    u32 size() const { return c.size(); }
    value_type& top() { return c.back(); }
    const value_type& top() const { return c.back(); }
    void push(const value_type& _X) { c.push_back(_X); }
    void pop() { c.pop_back(); }
    bool operator==(const _Myt& _X) const { return c==_X.c; }
    bool operator!=(const _Myt& _X) const { return !(*this==_X); }
    bool operator<(const _Myt& _X) const { return c<_X.c; }
    bool operator>(const _Myt& _X) const { return _X<*this; }
    bool operator<=(const _Myt& _X) const { return !(_X<*this); }
    bool operator>=(const _Myt& _X) const { return !(*this<_X); }

protected:
    _C c;
};

#define DEFINE_STACK(T, N) typedef xr_stack<T> N;
