///////////////////////////////////////////////////////////////
// BottleItem.h
// BottleItem - бутылка с напитком, которую можно разбить
///////////////////////////////////////////////////////////////

#pragma once

#include "FoodItem.h"

class CBottleItem : public CFoodItem
{
private:
    typedef CFoodItem inherited;

public:
    CBottleItem();
    virtual ~CBottleItem();

    virtual void Load(LPCSTR section);

    void OnEvent(NET_Packet& P, u16 type);

    virtual void Hit(SHit* pHDS);

    void BreakToPieces();

protected:
    //партиклы разбивания бутылки
    shared_str m_sBreakParticles;
    ref_sound sndBreaking;
};
