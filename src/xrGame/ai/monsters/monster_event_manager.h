#pragma once
#include "monster_event_manager_defs.h"

typedef fastdelegate::FastDelegate1<IEventData*> typeEvent;

class CMonsterEventManager
{
    // delayed remove
    struct event_struc
    {
        typeEvent delegate;
        bool need_remove;

        event_struc(typeEvent e) : delegate(e) { need_remove = false; }
    };

    using EVENT_VECTOR = xr_vector<event_struc>;
    using EVENT_MAP = xr_map<EEventType, EVENT_VECTOR>;

    EVENT_MAP m_event_storage;

public:
    CMonsterEventManager();
    ~CMonsterEventManager();

    void add_delegate(EEventType event, typeEvent delegate);
    void remove_delegate(EEventType event, typeEvent delegate);

    void raise(EEventType, IEventData* data = 0);

private:
    void clear();
};
