////////////////////////////////////////////////////////////////////////////
// Module : intrusive_ptr_inline.h
// Created : 30.07.2004
// Modified : 30.07.2004
// Author : Dmitriy Iassenev
// Description : Intrusive pointer template inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename object_type, typename base_type>
IC intrusive_ptr<object_type,base_type>::intrusive_ptr()
{
    m_object = 0;
}

template <typename object_type, typename base_type>
IC intrusive_ptr<object_type,base_type>::intrusive_ptr(object_type* rhs)
{
    m_object = 0;
    set(rhs);
}

template <typename object_type, typename base_type>
IC intrusive_ptr<object_type,base_type>::intrusive_ptr(self_type const& rhs)
{
    m_object = 0;
    set(rhs);
}

template <typename object_type, typename base_type>
IC intrusive_ptr<object_type,base_type>::~intrusive_ptr()
{
    STATIC_CHECK(result, Class_MUST_Be_Derived_From_The_Base);
    dec();
}

template <typename object_type, typename base_type>
IC void intrusive_ptr<object_type,base_type>::dec()
{
    if (!m_object)
        return;

    --m_object->base_type::m_ref_count;
    if (!m_object->base_type::m_ref_count)
        m_object->base_type::_release(m_object);
}

template <typename object_type, typename base_type>
IC typename intrusive_ptr<object_type,base_type>::self_type& intrusive_ptr<object_type,base_type>::operator= (object_type* rhs)
{
    set(rhs);
    return ((self_type&)*this);
}

template <typename object_type, typename base_type>
IC typename intrusive_ptr<object_type,base_type>::self_type& intrusive_ptr<object_type,base_type>::operator= (self_type const& rhs)
{
    set(rhs);
    return ((self_type&)*this);
}

template <typename object_type, typename base_type>
IC object_type& intrusive_ptr<object_type,base_type>::operator* () const
{
    VERIFY(m_object);
    return (*m_object);
}

template <typename object_type, typename base_type>
IC object_type* intrusive_ptr<object_type,base_type>::operator->() const
{
    VERIFY(m_object);
    return (m_object);
}

template <typename object_type, typename base_type>
IC bool intrusive_ptr<object_type,base_type>::operator! () const
{
    return (!m_object);
}

template <typename object_type, typename base_type>
IC void intrusive_ptr<object_type,base_type>::swap(self_type& rhs)
{
    object_type* tmp = m_object;
    m_object = rhs.m_object;
    rhs.m_object = tmp;
}

template <typename object_type, typename base_type>
IC bool intrusive_ptr<object_type,base_type>::equal(const self_type& rhs) const
{
    return (m_object == rhs.m_object);
}

template <typename object_type, typename base_type>
IC void intrusive_ptr<object_type,base_type>::set(object_type* rhs)
{
    if (m_object == rhs)
        return;
    dec();
    m_object = rhs;
    if (!m_object)
        return;
    ++m_object->m_ref_count;
}

template <typename object_type, typename base_type>
IC void intrusive_ptr<object_type,base_type>::set(self_type const& rhs)
{
    if (m_object == rhs.m_object)
        return;
    dec();
    m_object = rhs.m_object;
    if (!m_object)
        return;
    ++m_object->m_ref_count;
}

template <typename object_type, typename base_type>
IC const object_type* intrusive_ptr<object_type,base_type>::get() const
{
    return (m_object);
}

template <typename object_type, typename base_type>
IC bool operator== (intrusive_ptr<object_type,base_type> const& a, intrusive_ptr<object_type,base_type> const& b)
{
    return (a.get() == b.get());
}

template <typename object_type, typename base_type>
IC bool operator != (intrusive_ptr<object_type,base_type> const& a, intrusive_ptr<object_type,base_type> const& b)
{
    return (a.get() != b.get());
}

template <typename object_type, typename base_type>
IC bool operator< (intrusive_ptr<object_type,base_type> const& a, intrusive_ptr<object_type,base_type> const& b)
{
    return (a.get() < b.get());
}

template <typename object_type, typename base_type>
IC bool operator> (intrusive_ptr<object_type,base_type> const& a, intrusive_ptr<object_type,base_type> const& b)
{
    return (a.get() > b.get());
}

template <typename object_type, typename base_type>
IC void swap(intrusive_ptr<object_type,base_type>& lhs, intrusive_ptr<object_type,base_type>& rhs)
{
    lhs.swap(rhs);
}
