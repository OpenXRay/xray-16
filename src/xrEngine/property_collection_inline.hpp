////////////////////////////////////////////////////////////////////////////
// Module : property_collection_inline.hpp
// Created : 12.12.2007
// Modified : 27.12.2007
// Author : Dmitriy Iassenev
// Description : property collection template class inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef PROPERTY_COLLECTION_INLINE_HPP_INCLUDED
#define PROPERTY_COLLECTION_INLINE_HPP_INCLUDED

#define SPECIALIZATION template <typename container_type, typename holder_type>
#define PROPERTY_COLLECTION property_collection<container_type, holder_type>

SPECIALIZATION
inline PROPERTY_COLLECTION::property_collection(container_type* container, holder_type* holder, bool* changed)
    : m_container(*container), m_holder(*holder), m_changed(changed)
{
}

SPECIALIZATION
inline PROPERTY_COLLECTION::~property_collection()
{
    delete_data(m_container);
    // Msg ("container is destroyed");
}

SPECIALIZATION
inline holder_type& PROPERTY_COLLECTION::holder() const { return (m_holder); }
SPECIALIZATION
void PROPERTY_COLLECTION::clear()
{
    make_state_changed();
    m_container.clear();
    // Msg ("container is cleared");
}

SPECIALIZATION
u32 PROPERTY_COLLECTION::size() { return (m_container.size()); }

SPECIALIZATION
void PROPERTY_COLLECTION::insert(property_holder* holder, u32 const& position)
{
    // Msg ("insert into container");
    make_state_changed();

    VERIFY(position <= m_container.size());

    XRay::Editor::property_holder_holder* value_raw = holder->holder();
    VERIFY(value_raw);

    typedef typename container_type::value_type value_type;
    value_type value = dynamic_cast<value_type>(value_raw);
    VERIFY(value);

    m_container.insert(m_container.begin() + position, value);
}

SPECIALIZATION
void PROPERTY_COLLECTION::erase(u32 const& position)
{
    // Msg ("erase from container");
    make_state_changed();

    VERIFY(position < m_container.size());
    typename container_type::value_type value = m_container[position];
    m_container.erase(m_container.begin() + position);
    delete_data(value);
}

SPECIALIZATION
XRay::Editor::property_holder_base* PROPERTY_COLLECTION::item(u32 const& position) { return (m_container[position]->object()); }

SPECIALIZATION
inline PROPERTY_COLLECTION::predicate::predicate(property_holder* holder) : m_holder(holder) {}

SPECIALIZATION
inline bool PROPERTY_COLLECTION::predicate::operator()(typename container_type::value_type const& value) const
{
    return (m_holder == value->object());
}

SPECIALIZATION
int PROPERTY_COLLECTION::index(property_holder* holder)
{
    typedef typename container_type::iterator iterator_type;
    iterator_type i = std::find_if(m_container.begin(), m_container.end(), predicate(holder));
    if (i == m_container.end())
        return (-1);

    return (int(i - m_container.begin()));
}

SPECIALIZATION
void PROPERTY_COLLECTION::destroy(XRay::Editor::property_holder_base* holder) { delete_data(holder->holder()); }

SPECIALIZATION
inline PROPERTY_COLLECTION::unique_id_predicate::unique_id_predicate(LPCSTR id) : m_id(id) {}

SPECIALIZATION
inline bool PROPERTY_COLLECTION::unique_id_predicate::operator()(typename container_type::value_type const& value) const
{
    return (!xr_strcmp(m_id, value->id()));
}

SPECIALIZATION
bool PROPERTY_COLLECTION::unique_id(LPCSTR id) const
{
    return (std::find_if(m_container.begin(), m_container.end(), unique_id_predicate(id)) == m_container.end());
}

SPECIALIZATION
shared_str PROPERTY_COLLECTION::generate_unique_id(LPCSTR prefix) const
{
    for (u32 i = 0;; ++i)
    {
        string_path result;
        xr_strcpy(result, prefix);

        string_path number;
        R_ASSERT(!_itoa_s(i, number, 10));
        xr_strcat(result, number);

        if (!unique_id(result))
            continue;

        return (result);
    }
}

SPECIALIZATION
inline void PROPERTY_COLLECTION::make_state_changed()
{
    if (!m_changed)
        return;

    *m_changed = true;
}

#undef PROPERTY_COLLECTION
#undef SPECIALIZATION

#endif // #ifndef PROPERTY_COLLECTION_INLINE_HPP_INCLUDED
