////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_storage.cpp
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover storage class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"

#include "smart_cover_storage.h"
#include "smart_cover_description.h"

#include "Common/object_broker.h"

namespace smart_cover
{
storage::DescriptionPtr storage::description(shared_str const& table_id)
{
    collect_garbage();

    Descriptions::iterator found = std::find_if(m_descriptions.begin(), m_descriptions.end(),
        [=](smart_cover::description* const& ptr) { return (table_id._get() == ptr->table_id()._get()); });

    if (found != m_descriptions.end())
        return (*found);

    auto* description = xr_new<smart_cover::description>(table_id);
    m_descriptions.push_back(description);
    return (description);
}

storage::~storage()
{
#ifdef DEBUG
    Descriptions::const_iterator I = m_descriptions.begin();
    Descriptions::const_iterator E = m_descriptions.end();
    for (; I != E; ++I)
        VERIFY((*I)->released());
#endif // DEBUG
    delete_data(m_descriptions);
}

void storage::collect_garbage()
{
    struct garbage
    {
        static bool predicate(smart_cover::description* const& object)
        {
            if (!object->released())
                return (false);

            if (Device.dwTimeGlobal < object->m_last_time_dec + TIME_TO_REMOVE_GARBAGE)
                return (false);

            auto* temp = object;
            xr_delete(temp);
            return (true);
        }
    };

    m_descriptions.erase(
        std::remove_if(m_descriptions.begin(), m_descriptions.end(), &garbage::predicate), m_descriptions.end());
}
} // namespace smart_cover
