/***************************************************************************
 *   Copyright (C) 2018 - Vast & Ray of Hope Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/

#pragma once
#include <functional>
#include "xrDelegateArguments.h"

template<int>
struct delegate_argument
{};

namespace std
{
    template<int N>
    struct is_placeholder<delegate_argument<N>>
        : integral_constant<int, N + 1> // the one is important
    {};
}

template<typename Result>
class xrAbstractDelegate
{
public:
    xrAbstractDelegate() = default;

    xrAbstractDelegate(const xrAbstractDelegate& other)
        : d_handle(other.d_handle),
          d_functor_hash(other.d_functor_hash),
          d_function_hash(other.d_function_hash)
    {
    }

    xrAbstractDelegate(xrAbstractDelegate&& other) noexcept
        : d_handle(other.d_handle),
          d_functor_hash(other.d_functor_hash),
          d_function_hash(other.d_function_hash)
    {
    }

    xrAbstractDelegate& operator=(const xrAbstractDelegate& other)
    {
        if (this == &other)
            return *this;
        d_handle = other.d_handle;
        d_functor_hash = other.d_functor_hash;
        d_function_hash = other.d_function_hash;
        return *this;
    }

    xrAbstractDelegate& operator=(xrAbstractDelegate&& other) noexcept
    {
        if (this == &other)
            return *this;
        d_handle = other.d_handle;
        d_functor_hash = other.d_functor_hash;
        d_function_hash = other.d_function_hash;
        return *this;
    }

    virtual ~xrAbstractDelegate() = default;

    virtual Result invoke_args(xrDelegateArguments& args) const = 0;

    template<typename ... Args>
    Result invoke(Args ... args) const;

    bool operator==(const xrAbstractDelegate<Result>& delegate) const
    {
        return d_handle == delegate.d_handle
            && d_functor_hash == delegate.d_functor_hash
            && d_function_hash == delegate.d_function_hash;
    }

protected:
    void* d_handle = nullptr;
    size_t d_functor_hash = 0;
    size_t d_function_hash = 0;
};

template<typename Result, typename ... Args>
class xrDelegate;

template<typename Result, typename ... Args>
class xrDelegate<Result(Args...)> final : public xrAbstractDelegate<Result>
{
public:    
    using inherited = xrAbstractDelegate<Result>;
    using tuple_type = std::tuple<Args...>;
    using function_type = std::function<Result(Args...)>;

    xrDelegate() = default;

    xrDelegate(std::nullptr_t) {}

    xrDelegate(const xrDelegate& other)
        : inherited(other),
        d_function(other.d_function)
    {
    }

    xrDelegate(xrDelegate&& other) noexcept
        : inherited(std::move(other)),
        d_function(std::move(other.d_function))
    {
    }

    xrDelegate& operator=(const xrDelegate& other)
    {
        if (this == &other)
            return *this;
        inherited::operator =(other);
        d_function = other.d_function;
        return *this;
    }

    xrDelegate& operator=(xrDelegate&& other) noexcept
    {
        if (this == &other)
            return *this;
        inherited::operator =(std::move(other));
        d_function = std::move(other.d_function);
        return *this;
    }

    template<typename Fx>
    xrDelegate(Fx fx)
    {
        bind(fx);
    }

    template<typename Fx, typename Tx>
    xrDelegate(Fx fx, Tx tx)
    {
        bind(fx, tx);
    }

    ~xrDelegate() override = default;

    template<typename TClass, typename TFunction>
    void bind(TClass fx, TFunction tx)
    {
        using sequence = std::make_index_sequence<sizeof ... (Args)>;
        auto functor = bind_impl(sequence{}, tx, fx); 
        inherited::d_function_hash = typeid(TFunction).hash_code();
        inherited::d_functor_hash = typeid(decltype(functor)).hash_code();
        inherited::d_handle = fx;
        d_function = functor;
    }

    template<typename TFunction>
    void bind(TFunction fx)
    {
        using sequence = std::make_index_sequence<sizeof ... (Args)>;
        auto functor = bind_impl(sequence{}, fx);
        inherited::d_function_hash = typeid(TFunction).hash_code();
        inherited::d_functor_hash = typeid(decltype(functor)).hash_code();
        d_function = functor;
    }

    void reset()
    {
        d_function = nullptr;
        inherited::d_handle = nullptr;
        inherited::d_functor_hash = 0;
        inherited::d_function_hash = 0;
    }

    Result invoke_args(xrDelegateArguments& args) const override
    {
        if constexpr (sizeof ... (Args) > 0)
        {
            tuple_type& values = args.get<Args...>().values();
            using index_sequence = std::make_index_sequence<std::tuple_size<typename std::decay<tuple_type>::type>::value>;

            if constexpr (std::is_same_v<Result, void>)
                run(values, index_sequence{});
            else
                return run(values, index_sequence{});
        }
        else if constexpr (std::is_same_v<Result, void>)
            d_function();
        else
            return d_function();
    }

    Result invoke(Args ... args) const
    {
        if constexpr (std::is_same_v<Result, void>)
            d_function(std::forward<Args>(args)...);
        else
            return d_function(std::forward<Args>(args)...);
    }

    const function_type& get_function() const
    {
        return d_function;
    }

    bool empty() const
    {
        return d_function == nullptr;
    }

    xrDelegate& operator=(std::nullptr_t) noexcept
    {
        reset();
        return *this;
    }

    Result operator()(Args ... args) const
    {
        if constexpr (std::is_same_v<Result, void>)
            invoke(std::forward<Args>(args)...);
        else
            return invoke(std::forward<Args>(args)...);
    }

    bool operator==(const xrDelegate<Result(Args...)>& delegate) const
    {
        return inherited::d_handle == delegate.d_handle
            && inherited::d_functor_hash == delegate.d_functor_hash
            && inherited::d_function_hash == delegate.d_function_hash;
    }

    operator bool() const
    {
        return !empty();
    }

    bool operator!() const
    {
        return empty();
    }

private:
    template<std::size_t... index>
    Result run(tuple_type& tup, std::index_sequence<index...>) const
    {
        return d_function(std::forward<Args>(std::get<index>(tup))...);
    }

    template<typename ... Fx, size_t... Is>
    static auto bind_impl(std::integer_sequence<size_t, Is...>, Fx&& ... fx)
    {
        return std::bind(fx ..., delegate_argument<Is>{}...);
    }

    function_type d_function = nullptr;
};

template <typename Result>
template <typename ... Args>
Result xrAbstractDelegate<Result>::invoke(Args... args) const
{
    auto self = const_cast<xrAbstractDelegate<Result>*>(this);
    return static_cast<xrDelegate<Result(Args...)>*>(self)->invoke(args...);
}

#include "xrDelegateBinder.h"
