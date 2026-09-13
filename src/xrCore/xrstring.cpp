#include "stdafx.h"
#pragma hdrstop

#include "xrstring.h"
#include "xrstring_impl.h"
#include "xrstring_manager.h"

void shared_str::_dec() noexcept
{
    if (index_ == 0)
        return;

    if (!g_pStringContainer /* || !shared_str_initialized */)
    {
        index_ = 0;
        return;
    }

    str_value* p = g_pStringContainer->get_string(index_);
    if (p)
    {
        p->dwReference--;
        if (p->dwReference == 0)
        {
            index_ = 0;
        }
    }
}

void shared_str::_set(pcstr rhs)
{
    u32 new_index = g_pStringContainer->dock(rhs);
    if (new_index != 0)
    {
        str_value* v = g_pStringContainer->get_string(new_index);
        if (v)
            v->dwReference++;
    }
    _dec();
    index_ = new_index;
}

void shared_str::_set(shared_str const& rhs) noexcept
{
    u32 new_index = rhs.index_;
    if (new_index != 0)
    {
        str_value* v = g_pStringContainer->get_string(new_index);
        if (v)
            v->dwReference++;
    }
    _dec();
    index_ = new_index;
}

void shared_str::_set(std::nullptr_t) noexcept
{
    _dec();
    index_ = 0;
}

shared_str::shared_str(pcstr rhs)
{
    index_ = 0;
    _set(rhs);
}

shared_str::shared_str(shared_str const& rhs) noexcept
{
    index_ = 0;
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

const str_value* shared_str::_get() const
{
    if (index_ == 0)
        return nullptr;

    if (!g_pStringContainer /* || !shared_str_initialized */)
        return nullptr;

    return g_pStringContainer->get_string(index_);
}

char shared_str::operator[](size_t id)
{
    return _get()->value[id];
}

char shared_str::operator[](size_t id) const
{
    return _get()->value[id];
}

pcstr shared_str::c_str() const
{
    const str_value* p = _get();
    return p ? p->value : nullptr;
}

size_t shared_str::size() const
{
    const str_value* p = _get();
    return p ? p->dwLength : 0;
}

u32 shared_str::get_crc() const
{
    const str_value* p = _get();
    return p ? p->dwCRC : 0;
}

bool shared_str::equal(const shared_str& rhs) const
{
    return index_ == rhs.index_;
}
