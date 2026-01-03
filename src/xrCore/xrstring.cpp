#include "stdafx.h"
#pragma hdrstop

#include "xrstring.h"
#include "xrstring_impl.h"
#include "xrstring_manager.h"

void shared_str::_dec() noexcept
{
    if (nullptr == p_)
        return;
    p_->dwReference--;
    if (0 == p_->dwReference)
        p_ = nullptr;
}

void shared_str::_set(pcstr rhs)
{
    str_value* v = g_pStringContainer->dock(rhs);
    if (nullptr != v)
        v->dwReference++;
    _dec();
    p_ = v;
}

void shared_str::_set(shared_str const& rhs) noexcept
{
    str_value* v = rhs.p_;
    if (nullptr != v)
        v->dwReference++;
    _dec();
    p_ = v;
}

void shared_str::_set(std::nullptr_t) noexcept
{
    _dec();
    p_ = nullptr;
}

shared_str::shared_str(pcstr rhs)
{
    p_ = nullptr;
    _set(rhs);
}

shared_str::shared_str(shared_str const& rhs) noexcept
{
    p_ = nullptr;
    _set(rhs);
}

shared_str::~shared_str()
{
    _dec();
}

shared_str& shared_str::operator=(pcstr rhs)
{
    _set(rhs);
    return *this;
}

shared_str& shared_str::operator=(shared_str const& rhs) noexcept
{
    _set(rhs);
    return *this;
}

shared_str& shared_str::operator=(std::nullptr_t) noexcept
{
    _set(nullptr);
    return *this;
}

char shared_str::operator[](size_t id)
{
    return p_->value[id];
}

char shared_str::operator[](size_t id) const
{
    return p_->value[id];
}

pcstr shared_str::c_str() const
{
    return p_ ? p_->value : nullptr;
}

size_t shared_str::size() const
{
    if (nullptr == p_)
        return 0;
    return p_->dwLength;
}

u32 shared_str::get_crc() const
{
    return p_ ? p_->dwCRC : 0;
}
