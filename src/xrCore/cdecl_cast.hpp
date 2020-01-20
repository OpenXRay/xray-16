#pragma once

template <typename C, typename R, typename... A>
auto cdecl_cast(const C& c, R (C::*f)(A...) const) -> decltype(static_cast<R(__cdecl*)(A...)>(c))
{
    return static_cast<R(__cdecl*)(A...)>(c);
}

template <typename C, typename R, typename... A>
auto cdecl_cast(const C& c, R (C::*f)(A..., ...) const) -> decltype(static_cast<R(__cdecl*)(A..., ...)>(c))
{
    return static_cast<R(__cdecl*)(A..., ...)>(c);
}

template <typename TLambda>
auto cdecl_cast(const TLambda& c) -> decltype(cdecl_cast(c, &TLambda::operator()))
{
    return cdecl_cast(c, &TLambda::operator());
}
