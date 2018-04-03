#include "stdafx.h"

#include "FoodItem.h"
CFoodItem::CFoodItem() {}
CFoodItem::~CFoodItem() {}

void CFoodItem::net_Import(NET_Packet& P)
{
    CEatableItemObject::net_Import(P);
}

void CFoodItem::net_Export(NET_Packet& P)
{
    CEatableItemObject::net_Export(P);
}
