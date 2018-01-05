#pragma once
#include "xr_vector.h"

template <typename _Ty, class _C = xr_vector<_Ty>>
class xr_stack
{
public:
    using allocator_type = typename _C::allocator_type;
    using value_type = typename allocator_type::value_type;
    using size_type = typename allocator_type::size_type;
    using _Myt = xr_stack<_Ty, _C>;

    allocator_type get_allocator() const { return c.get_allocator(); }
    bool empty() const { return c.empty(); }
    u32 size() const { return c.size(); }
    value_type& top() { return c.back(); }
    const value_type& top() const { return c.back(); }
    void push(const value_type& _X) { c.push_back(_X); }
    void pop() { c.pop_back(); }
    bool operator==(const _Myt& _X) const { return c == _X.c; }
    bool operator!=(const _Myt& _X) const { return !(*this == _X); }
    bool operator<(const _Myt& _X) const { return c < _X.c; }
    bool operator>(const _Myt& _X) const { return _X < *this; }
    bool operator<=(const _Myt& _X) const { return !(_X < *this); }
    bool operator>=(const _Myt& _X) const { return !(*this < _X); }

protected:
    _C c;
};

#define DEFINE_STACK(T, N) using N = xr_stack<T>;
