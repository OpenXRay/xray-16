#pragma once

#include "xrScriptEngine/xrScriptEngine.hpp"
#include <type_traits>

namespace luabind
{
template<typename TResult>
class functor : public adl::object
{
public:
    functor() {}
    functor(const adl::object &obj) : adl::object(obj) {}

    template<typename... Args>
    TResult operator()(Args &&...args) const
    { return call_function<TResult>(*static_cast<const adl::object *>(this), std::forward<Args>(args)...); }
};

template<>
template<typename... Args>
void functor<void>::operator()(Args &&...args) const
{ call_function<void>(*static_cast<const adl::object *>(this), std::forward<Args>(args)...); }
}
