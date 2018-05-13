//////////////////////////////////////////////////////////////////////////
// relation_registry_defs.h:	реестр для хранения данных об отношении персонажа к
//								другим персонажам
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/object_interfaces.h"

//структура, описывающая отношение одного персонажа к другому или к группировке
struct SRelation
{
    SRelation();
    ~SRelation();
    CHARACTER_GOODWILL Goodwill() const { return m_iGoodwill; };
    void SetGoodwill(CHARACTER_GOODWILL new_goodwill) { m_iGoodwill = new_goodwill; };
private:
    //благосклонность
    CHARACTER_GOODWILL m_iGoodwill;
};

using PERSONAL_RELATION_MAP = xr_map<u16, SRelation>;
using COMMUNITY_RELATION_MAP = xr_map<CHARACTER_COMMUNITY_INDEX, SRelation>;

//структура, существует для каждого персонажа в игре
struct RELATION_DATA : public ISerializable
{
    virtual void clear();

    virtual void load(IReader&);
    virtual void save(IWriter&);

    //личные отношения
    PERSONAL_RELATION_MAP personal;
    //отношения с группировками
    COMMUNITY_RELATION_MAP communities;
};
