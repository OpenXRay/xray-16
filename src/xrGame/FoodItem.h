#pragma once

#include "eatable_item_object.h"

class CFoodItem : public CEatableItemObject
{
public:
    CFoodItem();
    virtual ~CFoodItem();

    virtual void net_Import(NET_Packet& P);					// import from server
    virtual void net_Export(NET_Packet& P);					// export to server
};
