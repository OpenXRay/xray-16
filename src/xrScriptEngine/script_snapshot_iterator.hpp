#pragma once

#include <type_traits>

#include "xrCommon/xr_smart_pointers.h"

#include "script_space.hpp"

namespace luabind
{
namespace detail
{
// Iterator over a copy of a container, which lives as long as an iterator over it.
template <typename Container>
class snapshot_iterator
{
public:
    snapshot_iterator(xr_shared_ptr<const Container> values, typename Container::const_iterator position)
        : m_values(std::move(values)), m_position(position) {}

    decltype(auto) operator*() const
    {
        return *m_position;
    }

    snapshot_iterator& operator++()
    {
        ++m_position;
        return *this;
    }

    bool operator!=(const snapshot_iterator& other) const
    {
        return m_position != other.m_position;
    }

private:
    xr_shared_ptr<const Container> m_values;
    typename Container::const_iterator m_position;
};

// Iterates a copy of the container taken when the loop starts, so the loop body may change the container.
struct snapshot_iterator_converter
{
    using type = snapshot_iterator_converter;

    template <typename Container>
    void to_lua(lua_State* luaState, const Container& container)
    {
        const xr_shared_ptr<const Container> values = xr_make_shared<Container>(container);
        make_range(luaState, snapshot_iterator<Container>(values, values->begin()), snapshot_iterator<Container>(values, values->end()));
    }
};

struct snapshot_iterator_policy
{
    template <typename T, typename Direction>
    struct specialize
    {
        static_assert(std::is_same_v<Direction, cpp_to_lua>, "Snapshot iterator policy can only convert from cpp to lua.");
        using type = snapshot_iterator_converter;
    };
};
} // namespace detail

namespace policy
{
using return_snapshot_iterator = converter_policy_injector<0, detail::snapshot_iterator_policy>;
} // namespace policy
} // namespace luabind
