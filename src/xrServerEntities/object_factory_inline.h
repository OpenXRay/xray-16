////////////////////////////////////////////////////////////////////////////
//	Module 		: object_factory_inline.h
//	Created 	: 27.05.2004
//  Modified 	: 27.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Object factory inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once
#include <algorithm>
#include "ai_space.h"
#include "xrCore/Events/Notifier.h"

IC const CObjectFactory& object_factory()
{
    if (!g_object_factory)
    {
        g_object_factory = new CObjectFactory();
        g_object_factory->init();

        class CResetEventCb : public CEventNotifierCallbackWithCid
        {
        public:
            CResetEventCb(CID cid) : CEventNotifierCallbackWithCid(cid) {}

            void ProcessEvent() override
            {
                xr_delete(g_object_factory);
                ai().Unsubscribe(GetCid(), CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
            }
        };

        ai().template Subscribe<CResetEventCb>(CAI_Space::EVENT_SCRIPT_ENGINE_RESET);
    }
    return (*g_object_factory);
}

IC bool CObjectFactory::CObjectItemPredicate::operator()(
    const CObjectItemAbstract* item1, const CObjectItemAbstract* item2) const
{
    return (item1->clsid() < item2->clsid());
}

IC bool CObjectFactory::CObjectItemPredicate::operator()(const CObjectItemAbstract* item, const CLASS_ID& clsid) const
{
    return (item->clsid() < clsid);
}

IC CObjectFactory::CObjectItemPredicateCLSID::CObjectItemPredicateCLSID(const CLASS_ID& clsid) : m_clsid(clsid) {}
IC bool CObjectFactory::CObjectItemPredicateCLSID::operator()(const CObjectItemAbstract* item) const
{
    return (m_clsid == item->clsid());
}

IC CObjectFactory::CObjectItemPredicateScript::CObjectItemPredicateScript(const shared_str& script_clsid_name)
    : m_script_clsid_name(script_clsid_name)
{
}

IC bool CObjectFactory::CObjectItemPredicateScript::operator()(const CObjectItemAbstract* item) const
{
    return (m_script_clsid_name == item->script_clsid());
}

IC const CObjectFactory::OBJECT_ITEM_STORAGE& CObjectFactory::clsids() const { return (m_clsids); }
#ifndef NO_XR_GAME
IC const CObjectItemAbstract& CObjectFactory::item(const CLASS_ID& clsid) const
{
    actualize();
    const_iterator I = std::lower_bound(clsids().begin(), clsids().end(), clsid, CObjectItemPredicate());
    VERIFY((I != clsids().end()) && ((*I)->clsid() == clsid));
    return (**I);
}
#else
IC const CObjectItemAbstract* CObjectFactory::item(const CLASS_ID& clsid, bool no_assert) const
{
    actualize();
    const_iterator I = std::lower_bound(clsids().begin(), clsids().end(), clsid, CObjectItemPredicate());
    if ((I == clsids().end()) || ((*I)->clsid() != clsid))
    {
        R_ASSERT(no_assert);
        return (nullptr);
    }
    return (*I);
}
#endif

IC void CObjectFactory::add(CObjectItemAbstract* item)
{
    const_iterator I;

    I = std::find_if(clsids().begin(), clsids().end(), CObjectItemPredicateCLSID(item->clsid()));
    if (I != clsids().end())
    {
        string16 temp;
        CLSID2TEXT(item->clsid(), temp);
        VERIFY2(0, make_string("clsid is duplicated : %s", temp));
    }

#ifndef NO_XR_GAME
    I = std::find_if(clsids().begin(), clsids().end(), CObjectItemPredicateScript(item->script_clsid()));
    VERIFY(I == clsids().end());
#endif

    m_actual = false;
    m_clsids.push_back(item);
}

IC int CObjectFactory::script_clsid(const CLASS_ID& clsid) const
{
    actualize();
    const_iterator I = std::lower_bound(clsids().begin(), clsids().end(), clsid, CObjectItemPredicate());
    VERIFY((I != clsids().end()) && ((*I)->clsid() == clsid));
    return (int(I - clsids().begin()));
}

#ifndef NO_XR_GAME
inline CObjectFactory::ClientObjectBaseClass* CObjectFactory::client_object(const CLASS_ID& clsid) const
{
    return (item(clsid).client_object());
}
#endif

inline CObjectFactory::ServerObjectBaseClass* CObjectFactory::server_object(const CLASS_ID& clsid, LPCSTR section) const
{
#ifndef NO_XR_GAME
    return (item(clsid).server_object(section));
#else
    const CObjectItemAbstract* object = item(clsid, true);
    return (object ? object->server_object(section) : 0);
#endif
}

IC void CObjectFactory::actualize() const
{
    if (m_actual)
        return;

    m_actual = true;
    std::sort(m_clsids.begin(), m_clsids.end(), CObjectItemPredicate());
}
