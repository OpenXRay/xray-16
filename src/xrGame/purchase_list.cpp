////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.cpp
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "purchase_list.h"
#include "InventoryOwner.h"
#include "GameObject.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "Level.h"

static float min_deficit_factor = .3f;

void CPurchaseList::process(CInifile& ini_file, LPCSTR section, CInventoryOwner& owner)
{
    owner.sell_useless_items();

    m_deficits.clear();

    const CGameObject& game_object = smart_cast<const CGameObject&>(owner);
    CInifile::Sect& S = ini_file.r_section(section);
    auto I = S.Data.cbegin();
    auto E = S.Data.cend();
    for (; I != E; ++I)
    {
        VERIFY3((*I).second.size(), "PurchaseList : cannot handle lines in section without values", section);

        string256 buffer;
        THROW3(_GetItemCount(*(*I).second) == 2, "Invalid parameters in section", section);
        {
            _GetItem(*(*I).second, 0, buffer);
            int tmp_i;
            xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp_i);
            _GetItem(*(*I).second, 1, buffer);
            float tmp_f;
            xr_from_chars(buffer, buffer + xr_strlen(buffer), tmp_f);
            process(game_object, (*I).first, tmp_i, tmp_f);
        }
    }
}

void CPurchaseList::process(
    const CGameObject& owner, const shared_str& name, const u32& count, const float& probability)
{
    VERIFY3(count, "Invalid count for section in the purchase list", *name);
    VERIFY3(!fis_zero(probability, EPS_S), "Invalid probability for section in the purchase list", *name);

    const Fvector& position = owner.Position();
    const u32& level_vertex_id = owner.ai_location().level_vertex_id();
    const ALife::_OBJECT_ID& id = owner.ID();
    CRandom random((u32)(CPU::QPC() & u32(-1)));
    u32 j = 0;
    for (u32 i = 0; i < count; ++i)
    {
        if (random.randF() > probability)
            continue;

        ++j;
        Level().spawn_item(*name, position, level_vertex_id, id, false);
    }

    DEFICITS::const_iterator I = m_deficits.find(name);
    VERIFY3(I == m_deficits.end(), "Duplicate section in the purchase list", *name);
    m_deficits.insert(std::make_pair(name, (float)count * probability / _max((float)j, min_deficit_factor)));
}
