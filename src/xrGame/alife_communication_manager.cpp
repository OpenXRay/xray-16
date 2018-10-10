////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_communication_manager.cpp
//	Created 	: 03.09.2003
//  Modified 	: 14.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife communication manager
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_communication_manager.h"

/**
#include "alife_communication_space.h"
#include "xrServer_objects_ALife_All.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "ai_debug.h"
#include "alife_human_brain.h"
#include "alife_human_object_handler.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

using namespace ALife;
using namespace ALifeCommunication;

#define KEEP_SORTED

#define PUSH_STACK(a,b,c,e,f) {\
    (e)[(f)].i1 = (a);\
    (e)[(f)].i2 = (b);\
    (e)[(f)].iCurrentSum = (c);\
    ++(f);\
}

#define POP_STACK(a,b,c,e,f) {\
    --(f);\
    (a) = (e)[(f)].i1;\
    (b) = (e)[(f)].i2;\
    (c) = (e)[(f)].iCurrentSum;\
}

class CSortItemByValuePredicate {
public:
    IC bool							operator()							(const CSE_ALifeInventoryItem
*tpALifeInventoryItem1, const CSE_ALifeInventoryItem *tpALifeInventoryItem2)  const
    {
        return						(tpALifeInventoryItem1->m_dwCost < tpALifeInventoryItem2->m_dwCost);
    };
};

class CSortByOwnerPredicate {
public:
    ALife::_OBJECT_ID				m_tParentID;

                                    CSortByOwnerPredicate	(ALife::_OBJECT_ID tParentID)
    {
        m_tParentID					= tParentID;
    }

    IC bool							operator()							(const CSE_ALifeInventoryItem
*tpALifeInventoryItem1, const CSE_ALifeInventoryItem *tpALifeInventoryItem2) const
    {
        if (tpALifeInventoryItem1->m_dwCost == tpALifeInventoryItem2->m_dwCost)
            if (tpALifeInventoryItem1->m_tPreviousParentID == m_tParentID)
                if (tpALifeInventoryItem2->m_tPreviousParentID == m_tParentID)
                    return				(tpALifeInventoryItem1->base()->ID < tpALifeInventoryItem2->base()->ID);
                else
                    return				(true);
            else
                if (tpALifeInventoryItem2->m_tPreviousParentID == m_tParentID)
                    return				(false);
                else
                    return				(tpALifeInventoryItem1->base()->ID < tpALifeInventoryItem2->base()->ID);
        else
            return						(tpALifeInventoryItem1->m_dwCost > tpALifeInventoryItem2->m_dwCost);
    }
};
/**/

CALifeCommunicationManager::CALifeCommunicationManager(IPureServer* server, LPCSTR section)
    : CALifeSimulatorBase(server, section)
{
    //	m_tpItems1.reserve			(MAX_STACK_DEPTH);
    //	m_tpItems2.reserve			(MAX_STACK_DEPTH);
    //	m_tpBlockedItems1.reserve	(MAX_STACK_DEPTH);
    //	m_tpBlockedItems2.reserve	(MAX_STACK_DEPTH);
    //	m_tpTrader1.reserve			(MAX_STACK_DEPTH);
    //	m_tpTrader1.reserve			(MAX_STACK_DEPTH);
    //	m_tpSums1.reserve			(MAX_STACK_DEPTH);
    //	m_tpSums2.reserve			(MAX_STACK_DEPTH);
}

/**
CALifeCommunicationManager::~CALifeCommunicationManager	()
{
}

#ifdef DEBUG
void CALifeCommunicationManager::vfPrintItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, ITEM_P_VECTOR &tpItemVector)
{
    Msg					("%s[%d]",tpALifeHumanAbstract->name_replace(),tpALifeHumanAbstract->m_dwMoney);
    ITEM_P_IT			I = tpItemVector.begin();
    ITEM_P_IT			E = tpItemVector.end();
    for ( ; I != E; ++I)
        Msg				(" %s",(*I)->base()->name_replace());
}

void CALifeCommunicationManager::vfPrintItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract)
{
    Msg					("%s[%d]",tpALifeHumanAbstract->name_replace(),tpALifeHumanAbstract->m_dwMoney);
    OBJECT_IT			I = tpALifeHumanAbstract->children.begin();
    OBJECT_IT			E = tpALifeHumanAbstract->children.end();
    for ( ; I != E; ++I)
        Msg				(" %s",objects().object(*I)->name_replace());
}
#endif

u32	 CALifeCommunicationManager::dwfComputeItemCost(ITEM_P_VECTOR &tpItemVector)
{
    u32					l_dwItemCost = 0;
    ITEM_P_IT			I = tpItemVector.begin();
    ITEM_P_IT			E = tpItemVector.end();
    for ( ; I != E; ++I)
        l_dwItemCost	+= (*I)->m_dwCost;
    return				(l_dwItemCost);
}

void CALifeCommunicationManager::vfRunFunctionByIndex(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, OBJECT_VECTOR
&tpBlockedItems, ITEM_P_VECTOR &tpItems, int i, int &j)
{
    sort
(m_temp_item_vector.begin(),m_temp_item_vector.end(),CSortByOwnerPredicate(tpALifeHumanAbstract->ID));
    switch (i) {
        case 0 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_food		(&tpBlockedItems);
            break;
        }
        case 1 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_weapon		(eWeaponPriorityTypeKnife,&tpBlockedItems);
            break;
        }
        case 2 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_weapon
(eWeaponPriorityTypeSecondary,&tpBlockedItems);
            break;
        }
        case 3 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_weapon (eWeaponPriorityTypePrimary,&tpBlockedItems);
            break;
        }
        case 4 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_weapon (eWeaponPriorityTypeGrenade,&tpBlockedItems);
            break;
        }
        case 5 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_medikit		(&tpBlockedItems);
            break;
        }
        case 6 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_detector	(&tpBlockedItems);
            break;
        }
        case 7 : {
            j	= tpALifeHumanAbstract->brain().objects().choose_equipment	(&tpBlockedItems);
            break;
        }
        default :																	NODEFAULT;
    }
}

void CALifeCommunicationManager::vfAssignItemParents(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, int iItemCount)
{
    ALife::_OBJECT_ID		*temp_children = (ALife::_OBJECT_ID*)alloca(sizeof(ALife::_OBJECT_ID)*iItemCount);
    std::copy				(tpALifeHumanAbstract->children.begin() + tpALifeHumanAbstract->children.size() -
iItemCount,tpALifeHumanAbstract->children.end(),temp_children);
    tpALifeHumanAbstract->children.resize(tpALifeHumanAbstract->children.size() - iItemCount);
    ALife::_OBJECT_ID		*I = temp_children;
    ALife::_OBJECT_ID		*E = temp_children + iItemCount;
    for ( ; I != E; ++I) {
        CSE_ALifeInventoryItem *l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I));
        tpALifeHumanAbstract->attach(l_tpALifeInventoryItem,true,true);
        R_ASSERT			(tpALifeHumanAbstract->brain().m_dwTotalMoney >= l_tpALifeInventoryItem->m_dwCost);
        tpALifeHumanAbstract->brain().m_dwTotalMoney	-= l_tpALifeInventoryItem->m_dwCost;
    }
}

void CALifeCommunicationManager::vfAttachOwnerItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, ITEM_P_VECTOR
&tpItemVector, ITEM_P_VECTOR &tpOwnItems)
{
    ITEM_P_IT				I = tpItemVector.begin();
    ITEM_P_IT				E = tpItemVector.end();
    for ( ; I != E; ++I)
//		if ((std::find(tpOwnItems.begin(),tpOwnItems.end(),*I) != tpOwnItems.end()) &&
tpALifeHumanAbstract->bfCanGetItem(*I))
        if (std::binary_search(tpOwnItems.begin(),tpOwnItems.end(),*I))
            tpALifeHumanAbstract->attach(*I,true);
}

int  CALifeCommunicationManager::ifComputeBalance(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, ITEM_P_VECTOR
&tpItemVector)
{
    int				l_iDebt = 0;
    OBJECT_VECTOR	&l_tpChildren = tpALifeHumanAbstract->children;
    ITEM_P_IT		I = tpItemVector.begin();
    ITEM_P_IT		E = tpItemVector.end();
    for ( ; I != E; ++I)
        if (std::find(l_tpChildren.begin(),l_tpChildren.end(),(*I)->base()->ID) != l_tpChildren.end())
            l_iDebt	-= (*I)->m_dwCost;
    return(l_iDebt);
}

void CALifeCommunicationManager::vfRestoreItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, ITEM_P_VECTOR
&tpItemVector)
{
#ifndef FAST_OWNERSHIP
    tpALifeHumanAbstract->vfInitInventory();
#endif
    {
        ITEM_P_IT		I = tpItemVector.begin();
        ITEM_P_IT		E = tpItemVector.end();
#ifdef FAST_OWNERSHIP
        OBJECT_IT		i = tpALifeHumanAbstract->children.begin();
#endif
        for ( ; I != E; ++I) {
#ifndef FAST_OWNERSHIP
            (*I)->base()->ID_Parent = 0xffff;
            graph().attach(*tpALifeHumanAbstract,*I,smart_cast<CSE_ALifeDynamicObject*>(*I)->m_tGraphID);
#else
            *i = (*I)->base()->ID;
            ++i;
#endif
        }
    }
}

#ifdef FAST_OWNERSHIP
void CALifeCommunicationManager::vfAttachGatheredItems(CSE_ALifeTraderAbstract *tpALifeTraderAbstract1,
CSE_ALifeTraderAbstract *tpALifeTraderAbstract2, OBJECT_VECTOR &tpObjectVector)
#else
void CALifeCommunicationManager::vfAttachGatheredItems(CSE_ALifeTraderAbstract *tpALifeTraderAbstract1, OBJECT_VECTOR
&tpObjectVector)
#endif
{
    tpObjectVector.clear();
    tpObjectVector.insert(tpObjectVector.end(),tpALifeTraderAbstract1->base()->children.begin(),tpALifeTraderAbstract1->base()->children.end());
    tpALifeTraderAbstract1->base()->children.clear();
    tpALifeTraderAbstract1->vfInitInventory();
    OBJECT_IT			I = tpObjectVector.begin();
    OBJECT_IT			E = tpObjectVector.end();
    for ( ; I != E; ++I) {
#ifndef FAST_OWNERSHIP
        CSE_ALifeDynamicObject	*l_tpALifeDynamicObject = objects().object(*I);
        l_tpALifeDynamicObject->ID_Parent = 0xffff;
        graph().attach
(*tpALifeTraderAbstract1->base(),smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeDynamicObject),l_tpALifeDynamicObject->m_tGraphID);
#else
        CSE_ALifeInventoryItem	*l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I));
        if (l_tpALifeInventoryItem->m_tPreviousParentID != tpALifeTraderAbstract1->base()->ID) {
            R_ASSERT							(l_tpALifeInventoryItem->m_tPreviousParentID ==
tpALifeTraderAbstract2->base()->ID);
//			tpALifeTraderAbstract2->detach		(l_tpALifeInventoryItem,0,false);
        }
        tpALifeTraderAbstract1->attach			(l_tpALifeInventoryItem,true);
#endif
    }
}

void CALifeCommunicationManager::vfFillTraderVector(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, int iItemCount,
ITEM_P_VECTOR &tpItemVector)
{
    tpItemVector.clear	();
    OBJECT_IT			I = tpALifeHumanAbstract->children.end() - iItemCount;
    OBJECT_IT			E = tpALifeHumanAbstract->children.end();
    for ( ; I != E; ++I)
        tpItemVector.push_back(smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I)));
}

void CALifeCommunicationManager::vfGenerateSums	(ITEM_P_VECTOR &tpTrader, INT_VECTOR &tpSums)
{
    int					l_iStackPointer = 0;
    tpSums.clear		();
    tpSums.push_back	(0);
    for (int i=0, n=tpTrader.size(); i<n; ++i) {

        PUSH_STACK		(i+1,n-1,0,m_tpStack1,l_iStackPointer);

        for (;l_iStackPointer;) {
            int			i1,i2,iCurrentSum;

            POP_STACK	(i1,i2,iCurrentSum,m_tpStack1,l_iStackPointer);

            if (!i1) {
#ifdef KEEP_SORTED
                INT_IT I = std::lower_bound(tpSums.begin(),tpSums.end(),iCurrentSum);
                if ((tpSums.end() == I) || (*I != iCurrentSum)) {
                    tpSums.insert	(I,iCurrentSum);
                    if (tpSums.size() >= SUM_COUNT_THRESHOLD)
                        return;
                }
#else
                INT_IT I = std::find(tpSums.begin(),tpSums.end(),iCurrentSum);
                if (tpSums.end() == I) {
                    tpSums.push_back(iCurrentSum);
                    if (tpSums.size() >= SUM_COUNT_THRESHOLD) {
                        sort		(tpSums.begin(),tpSums.end());
                        return;
                    }
                }
#endif
                continue;
            }

            for (int i=i2; i>=0; --i)
                PUSH_STACK	(i1 - 1, i - 1, iCurrentSum + tpTrader[i]->m_dwCost,m_tpStack1,l_iStackPointer);
        }
    }
#ifndef KEEP_SORTED
    sort					(tpSums.begin(),tpSums.end());
#endif
}

bool CALifeCommunicationManager::bfGetItemIndexes	(ITEM_P_VECTOR &tpTrader, int iSum1, INT_VECTOR &tpIndexes,
SSumStackCell *tpStack, int iStartI, int iStackPointer)
{
    for (int j=iStartI, n=tpTrader.size(); j<n; ++j) {

        PUSH_STACK			(j+1,n-1,0,tpStack,iStackPointer);
        tpIndexes.resize	(j + 2);

        for (;iStackPointer;) {
            int				i1,i2,iCurrentSum;

            POP_STACK		(i1,i2,iCurrentSum,tpStack,iStackPointer);

            tpIndexes[i1]	= i2 + 1;

            if (!i1) {
                if (iCurrentSum == iSum1) {
                    tpIndexes.resize(j + 1);
                    return	(true);
                }
                continue;
            }

            if (iCurrentSum >= iSum1)
                continue;

            for (int i=i2; i>=0; --i)
                PUSH_STACK	(i1 - 1, i - 1, iCurrentSum + tpTrader[i]->m_dwCost,tpStack,iStackPointer);
        }
    }
    return					(false);
}

bool CALifeCommunicationManager::bfCheckForInventoryCapacity(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
ITEM_P_VECTOR &tpTrader1, INT_VECTOR &tpIndexes1, CSE_ALifeHumanAbstract *tpALifeHumanAbstract2, ITEM_P_VECTOR
&tpTrader2, INT_VECTOR &tpIndexes2)
{
    {
        INT_IT				I = tpIndexes2.begin();
        INT_IT				E = tpIndexes2.end();
        for ( ; I != E; ++I) {
            tpALifeHumanAbstract1->children.push_back(tpTrader2[*I]->base()->ID);
            tpALifeHumanAbstract2->children.erase(std::find(tpALifeHumanAbstract2->children.begin(),tpALifeHumanAbstract2->children.end(),tpTrader2[*I]->base()->ID));
        }
    }
    {
        INT_IT				I = tpIndexes1.begin();
        INT_IT				E = tpIndexes1.end();
        for ( ; I != E; ++I) {
            tpALifeHumanAbstract2->children.push_back(tpTrader1[*I]->base()->ID);
            tpALifeHumanAbstract1->children.erase(std::find(tpALifeHumanAbstract1->children.begin(),tpALifeHumanAbstract1->children.end(),tpTrader1[*I]->base()->ID));
        }
    }

    bool					l_bResult = tpALifeHumanAbstract1->brain().objects().can_take_item(0) &&
tpALifeHumanAbstract2->brain().objects().can_take_item(0);

    if (!l_bResult) {
        {
            INT_IT				I = tpIndexes1.begin();
            INT_IT				E = tpIndexes1.end();
            for ( ; I != E; ++I)
                tpALifeHumanAbstract1->children.push_back(tpTrader1[*I]->base()->ID);
        }
        {
            INT_IT				I = tpIndexes2.begin();
            INT_IT				E = tpIndexes2.end();
            for ( ; I != E; ++I)
                tpALifeHumanAbstract2->children.push_back(tpTrader2[*I]->base()->ID);
        }
        return				(false);
    }
    else
        return				(true);
}

bool CALifeCommunicationManager::bfCheckForInventoryCapacity(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
ITEM_P_VECTOR &tpTrader1, int iSum1, int iMoney1, CSE_ALifeHumanAbstract *tpALifeHumanAbstract2, ITEM_P_VECTOR
&tpTrader2, int iSum2, int iMoney2, int iBalance)
{
    INT_VECTOR				l_tpIndexes1, l_tpIndexes2;
    int						l_iStartI1 = 0, l_iStackPointer1 = 0, l_iStartI2 = 0, l_iStackPointer2 = 0;
    for (;;) {
        l_tpIndexes1.clear	();

        if (iSum1)
            if (!bfGetItemIndexes(tpTrader1,iSum1,l_tpIndexes1,m_tpStack1,l_iStartI1,l_iStackPointer1))
                return		(false);

        for (;;) {
            l_tpIndexes2.clear();

            if (iSum2 && !bfGetItemIndexes(tpTrader2,iSum2,l_tpIndexes2,m_tpStack2,l_iStartI2,l_iStackPointer2))
                return	(false);

            if
(!bfCheckForInventoryCapacity(tpALifeHumanAbstract1,tpTrader1,l_tpIndexes1,tpALifeHumanAbstract2,tpTrader2,l_tpIndexes2))
                continue;

#ifdef DEBUG
            string4096		S;
            char			*S1 = S;
            if (psAI_Flags.test(aiALife)) {
                S1				+= xr_sprintf(S1,"%s -> ",tpALifeHumanAbstract1->name_replace());

                if (iSum1) {
                    for (int i=0, n=l_tpIndexes1.size(); i<n ;++i)
                        S1		+= xr_sprintf(S1,"%3d",l_tpIndexes1[i]);
                }
            }
#endif
            if (iSum1 < iBalance + iSum2) {
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    S1			+= xr_sprintf(S1," + $%d",iBalance + iSum2 - iSum1);
                }
#endif
                R_ASSERT	(int(tpALifeHumanAbstract1->m_dwMoney) >= iBalance + iSum2 - iSum1);
                tpALifeHumanAbstract1->m_dwMoney -= iBalance + iSum2 - iSum1;
                tpALifeHumanAbstract2->m_dwMoney += iBalance + iSum2 - iSum1;
            }

#ifdef DEBUG
            if (psAI_Flags.test(aiALife)) {
                S1				+= xr_sprintf(S1,"\n%s -> ",tpALifeHumanAbstract2->name_replace());

                if (iSum2) {
                    for (int i=0, n=l_tpIndexes2.size(); i<n ;++i)
                        S1		+= xr_sprintf(S1,"%3d",l_tpIndexes2[i]);
                }
            }
#endif

            if (iSum1 > iBalance + iSum2) {
#ifdef DEBUG
                if (psAI_Flags.test(aiALife)) {
                    S1			+= xr_sprintf(S1," + $%d",iSum1 - iBalance - iSum2);
                }
#endif
                R_ASSERT	(int(tpALifeHumanAbstract2->m_dwMoney) >= iSum1 - iBalance - iSum2);
                tpALifeHumanAbstract1->m_dwMoney += iSum1 - iBalance - iSum2;
                tpALifeHumanAbstract2->m_dwMoney -= iSum1 - iBalance - iSum2;
            }

#ifdef DEBUG
            if (psAI_Flags.test(aiALife)) {
                Msg			("%s\n Can trade!",S);
            }
#endif
            return			(true);
        }
    }
}

bool CALifeCommunicationManager::bfCheckForTrade	(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1, ITEM_P_VECTOR
&tpTrader1, INT_VECTOR &tpSums1, int iMoney1, CSE_ALifeHumanAbstract *tpALifeHumanAbstract2, ITEM_P_VECTOR &tpTrader2,
INT_VECTOR &tpSums2, int iMoney2, int iBalance)
{
    INT_IT					I	= tpSums1.begin();
    INT_IT					E	= tpSums1.end();
    INT_IT					II	= tpSums2.begin();
    INT_IT					EE	= tpSums2.end();

    for ( ; I != E; ++I) {
        if (*I + iMoney1 < iBalance)
            continue;

        if (*I < iBalance) {
            if
(bfCheckForInventoryCapacity(tpALifeHumanAbstract1,tpTrader1,*I,iMoney1,tpALifeHumanAbstract2,tpTrader2,0,iMoney2,iBalance))
                break;
        }
        else {
            bool			l_bOk = false;
            II				= tpSums2.begin();
            for ( ; II != EE; ++II)
                if (*I >= *II + iBalance)
                    if (*I - *II - iBalance <= iMoney2) {
                        if
(bfCheckForInventoryCapacity(tpALifeHumanAbstract1,tpTrader1,*I,iMoney1,tpALifeHumanAbstract2,tpTrader2,*II,iMoney2,iBalance))
{
                            l_bOk = true;
                            break;
                        }
                    }
                    else
                        continue;
                else
                    if (*I + iMoney1 >= *II + iBalance) {
                        if
(bfCheckForInventoryCapacity(tpALifeHumanAbstract1,tpTrader1,*I,iMoney1,tpALifeHumanAbstract2,tpTrader2,*II,iMoney2,iBalance))
{
                            l_bOk = true;
                            break;
                        }
                    }
                    else
                        break;
            if (l_bOk)
                break;
        }
    }

    if (I == E) {
#ifdef DEBUG
        if (psAI_Flags.test(aiALife)) {
            Msg				("Can't trade!\n");
        }
#endif
        return				(false);
    }
    else
        return				(true);
}

bool CALifeCommunicationManager::bfCheckIfCanNullTradersBalance(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1,
CSE_ALifeHumanAbstract *tpALifeHumanAbstract2, int iItemCount1, int iItemCount2, int iBalance)
{
    if (!iBalance) {
#ifdef DEBUG
        if (psAI_Flags.test(aiALife)) {
            Msg			("Balance is null");
        }
#endif
        return			(true);
    }

    if (iBalance < 0) {
        if (int(tpALifeHumanAbstract1->m_dwMoney) >= -iBalance) {
            tpALifeHumanAbstract1->m_dwMoney += iBalance;
            tpALifeHumanAbstract2->m_dwMoney -= iBalance;
#ifdef DEBUG
            if (psAI_Flags.test(aiALife)) {
                Msg		("Balance is covered by money");
            }
#endif
            return	(true);
        }
    }
    else
        if (int(tpALifeHumanAbstract2->m_dwMoney) >= iBalance) {
            tpALifeHumanAbstract1->m_dwMoney += iBalance;
            tpALifeHumanAbstract2->m_dwMoney -= iBalance;
#ifdef DEBUG
            if (psAI_Flags.test(aiALife)) {
                Msg			("Balance is covered by money");
            }
#endif
            return	(true);
        }

        vfFillTraderVector	(tpALifeHumanAbstract1,iItemCount1,m_tpTrader1);
        vfFillTraderVector	(tpALifeHumanAbstract2,iItemCount2,m_tpTrader2);

        sort				(m_tpTrader1.begin(),m_tpTrader1.end(),CSortItemByValuePredicate());
        sort				(m_tpTrader2.begin(),m_tpTrader2.end(),CSortItemByValuePredicate());

#ifdef DEBUG
        if (psAI_Flags.test(aiALife)) {
            {
                string4096		S;
                char			*S1 = S;
                S1				+= xr_sprintf(S1,"%s [%5d]:
",tpALifeHumanAbstract1->name_replace(),tpALifeHumanAbstract1->m_dwMoney);
                for (int i=0, n=m_tpTrader1.size(); i<n; ++i)
                    S1			+= xr_sprintf(S1,"%6d",m_tpTrader1[i]->m_dwCost);
                Msg				("%s",S);
            }
            {
                string4096		S;
                char			*S1 = S;
                S1				+= xr_sprintf(S1,"%s [%5d]:
",tpALifeHumanAbstract2->name_replace(),tpALifeHumanAbstract2->m_dwMoney);
                for (int i=0, n=m_tpTrader2.size(); i<n; ++i)
                    S1			+= xr_sprintf(S1,"%6d",m_tpTrader2[i]->m_dwCost);
                Msg				("%s",S);
            }
            Msg					("Balance : %6d",iBalance);
        }
#endif

        vfGenerateSums		(m_tpTrader1,m_tpSums1);
        vfGenerateSums		(m_tpTrader2,m_tpSums2);

#ifdef DEBUG
        if (psAI_Flags.test(aiALife)) {
            {
                string4096		S;
                char			*S1 = S;
                S1				+= xr_sprintf(S1,"%s : ",tpALifeHumanAbstract1->name_replace());
                INT_IT			I = m_tpSums1.begin();
                INT_IT			E = m_tpSums1.end();
                for ( ; I != E; ++I)
                    S1			+= xr_sprintf(S1,"%6d",*I);
                Msg				("%s",S);
            }
            {
                string4096		S;
                char			*S1 = S;
                S1				+= xr_sprintf(S1,"%s : ",tpALifeHumanAbstract2->name_replace());
                INT_IT			I = m_tpSums2.begin();
                INT_IT			E = m_tpSums2.end();
                for ( ; I != E; ++I)
                    S1			+= xr_sprintf(S1,"%6d",*I);
                Msg				("%s",S);
            }
        }
#endif

        if (iBalance < 0)
            return			(bfCheckForTrade(tpALifeHumanAbstract1, m_tpTrader1, m_tpSums1,
tpALifeHumanAbstract1->m_dwMoney, tpALifeHumanAbstract2, m_tpTrader2, m_tpSums2, tpALifeHumanAbstract2->m_dwMoney,
_abs(iBalance)));
        else
            return			(bfCheckForTrade(tpALifeHumanAbstract2, m_tpTrader2, m_tpSums2,
tpALifeHumanAbstract2->m_dwMoney, tpALifeHumanAbstract1, m_tpTrader1, m_tpSums1, tpALifeHumanAbstract1->m_dwMoney,
iBalance));
}

void CALifeCommunicationManager::vfAppendBlockedItems(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, OBJECT_VECTOR
&tpObjectVector1, OBJECT_VECTOR &tpObjectVector2, int l_iItemCount1)
{
    OBJECT_IT			I = tpALifeHumanAbstract->children.end() - l_iItemCount1;
    OBJECT_IT			E = tpALifeHumanAbstract->children.end();
    for ( ; I != E; ++I)
        if (std::find(tpObjectVector2.begin(),tpObjectVector2.end(),*I) == tpObjectVector2.end())
            tpObjectVector1.push_back(*I);
}

void CALifeCommunicationManager::vfPerformTrading(CSE_ALifeHumanAbstract *tpALifeHumanAbstract1, CSE_ALifeHumanAbstract
*tpALifeHumanAbstract2)
{
//	VERIFY					(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY					(tpALifeHumanAbstract2->check_inventory_consistency());

    m_tpItems1.clear	();
    m_tpItems2.clear	();

    append_item_vector	(tpALifeHumanAbstract1->children,m_tpItems1);
    append_item_vector	(tpALifeHumanAbstract2->children,m_tpItems2);

#ifdef DEBUG
    if (psAI_Flags.test(aiALife)) {
        vfPrintItems	(tpALifeHumanAbstract1,m_tpItems1);
        vfPrintItems	(tpALifeHumanAbstract2,m_tpItems2);
    }
#endif
    tpALifeHumanAbstract1->brain().m_dwTotalMoney = dwfComputeItemCost(m_tpItems1) + tpALifeHumanAbstract1->m_dwMoney;
    tpALifeHumanAbstract2->brain().m_dwTotalMoney = dwfComputeItemCost(m_tpItems2) + tpALifeHumanAbstract2->m_dwMoney;

    if (!(tpALifeHumanAbstract1->brain().m_dwTotalMoney*tpALifeHumanAbstract2->brain().m_dwTotalMoney)) {
        tpALifeHumanAbstract1->brain().m_dwTotalMoney = u32(-1);
        tpALifeHumanAbstract2->brain().m_dwTotalMoney = u32(-1);
#ifdef DEBUG
        if (psAI_Flags.test(aiALife)) {
            Msg			("There is no money and valuable items to trade");
        }
#endif
        return;
    }

    m_temp_item_vector		= m_tpItems1;
    m_temp_item_vector.insert(m_temp_item_vector.end(),m_tpItems2.begin(),m_tpItems2.end());

    sort				(m_tpItems1.begin(),m_tpItems1.end());
    sort				(m_tpItems2.begin(),m_tpItems2.end());

#ifdef FAST_OWNERSHIP
    tpALifeHumanAbstract1->vfDetachAll(true);
    tpALifeHumanAbstract2->vfDetachAll(true);
#else
    tpALifeHumanAbstract1->vfDetachAll();
    tpALifeHumanAbstract2->vfDetachAll();
#endif
    for (int j=0, k=0; j<8; ++j) {
        if (m_temp_item_vector.empty() || !(tpALifeHumanAbstract1->brain().m_dwTotalMoney +
tpALifeHumanAbstract2->brain().m_dwTotalMoney))
            break;
        int				l_iItemCount1 = 0, l_iItemCount2 = 0;
        switch (k) {
            case 0 : {
                vfRunFunctionByIndex(tpALifeHumanAbstract1,m_tpBlockedItems1,m_tpItems1,j,l_iItemCount1);
                vfRunFunctionByIndex(tpALifeHumanAbstract2,m_tpBlockedItems2,m_tpItems2,j,l_iItemCount2);
                break;
            }
            case 1 : {
                vfRunFunctionByIndex(tpALifeHumanAbstract1,m_tpBlockedItems1,m_tpItems1,j,l_iItemCount1);
                break;
            }
            case 2 : {
                vfRunFunctionByIndex(tpALifeHumanAbstract2,m_tpBlockedItems2,m_tpItems2,j,l_iItemCount2);
                break;
            }
            case 3 : {
                vfRunFunctionByIndex(tpALifeHumanAbstract1,m_tpBlockedItems1,m_tpItems1,j,l_iItemCount1);

                m_tpBlockedItems2.clear();
                m_tpBlockedItems2.insert(m_tpBlockedItems2.end(),tpALifeHumanAbstract1->children.end() -
l_iItemCount1,tpALifeHumanAbstract1->children.end());

                vfRunFunctionByIndex(tpALifeHumanAbstract2,m_tpBlockedItems2,m_tpItems2,j,l_iItemCount2);

                m_tpBlockedItems1.clear();
                m_tpBlockedItems1.insert(m_tpBlockedItems1.end(),tpALifeHumanAbstract2->children.end() -
l_iItemCount2,tpALifeHumanAbstract2->children.end());

                tpALifeHumanAbstract1->children.resize(tpALifeHumanAbstract1->children.size() - l_iItemCount1);

                vfRunFunctionByIndex(tpALifeHumanAbstract1,m_tpBlockedItems1,m_tpItems1,j,l_iItemCount1);
                break;
            }
        }

        m_tpBlockedItems1.clear();
        m_tpBlockedItems2.clear();

        if (l_iItemCount1*l_iItemCount2) {
            OBJECT_IT			I = tpALifeHumanAbstract1->children.end() - l_iItemCount1, J;
            OBJECT_IT			E = tpALifeHumanAbstract1->children.end();
            for ( ; I != E; ++I) {
                J				= std::find(tpALifeHumanAbstract2->children.end() -
l_iItemCount2,tpALifeHumanAbstract2->children.end(),*I);
                if (tpALifeHumanAbstract2->children.end() != J) {
                    if
(std::binary_search(m_tpItems1.begin(),m_tpItems1.end(),smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I))))
                        m_tpBlockedItems2.push_back(*I);
                    else {
                        R_ASSERT2(std::binary_search(m_tpItems2.begin(),m_tpItems2.end(),smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I))),"Unknown
item parent");
                        m_tpBlockedItems1.push_back(*I);
                    }
                }
            }
        }

        if (m_tpBlockedItems1.empty() && m_tpBlockedItems2.empty()) {
            if (l_iItemCount1)
                vfAssignItemParents	(tpALifeHumanAbstract1,l_iItemCount1);
            if (l_iItemCount2)
                vfAssignItemParents	(tpALifeHumanAbstract2,l_iItemCount2);
            if (l_iItemCount1 + l_iItemCount2) {
                ITEM_P_IT			I =
remove_if(m_temp_item_vector.begin(),m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
                m_temp_item_vector.erase(I,m_temp_item_vector.end());
            }
            k					= 0;
        }
        else {
            if (!m_tpBlockedItems1.empty())
                if (!m_tpBlockedItems2.empty()) {
                    vfAppendBlockedItems(tpALifeHumanAbstract1,m_tpBlockedItems1,m_tpBlockedItems2,l_iItemCount1);
                    vfAppendBlockedItems(tpALifeHumanAbstract2,m_tpBlockedItems2,m_tpBlockedItems1,l_iItemCount2);
                    tpALifeHumanAbstract1->children.resize(tpALifeHumanAbstract1->children.size() - l_iItemCount1);
                    tpALifeHumanAbstract2->children.resize(tpALifeHumanAbstract2->children.size() - l_iItemCount2);
                    k = 3;
                }
                else {
                    tpALifeHumanAbstract1->children.resize(tpALifeHumanAbstract1->children.size() - l_iItemCount1);
                    vfAssignItemParents	(tpALifeHumanAbstract2,l_iItemCount2);
                    ITEM_P_IT			I =
remove_if(m_temp_item_vector.begin(),m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
                    m_temp_item_vector.erase(I,m_temp_item_vector.end());
                    k = 1;
                }
            else {
                tpALifeHumanAbstract2->children.resize(tpALifeHumanAbstract2->children.size() - l_iItemCount2);
                k = 2;
                vfAssignItemParents	(tpALifeHumanAbstract1,l_iItemCount1);
                ITEM_P_IT			I =
remove_if(m_temp_item_vector.begin(),m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
                m_temp_item_vector.erase(I,m_temp_item_vector.end());
            }
            --j;
        }
    }

//	VERIFY				(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY				(tpALifeHumanAbstract2->check_inventory_consistency());

    int					l_iItemCount1 = tpALifeHumanAbstract1->children.size();
    int					l_iItemCount2 = tpALifeHumanAbstract2->children.size();

    vfAttachOwnerItems	(tpALifeHumanAbstract1,m_temp_item_vector,m_tpItems1);
    vfAttachOwnerItems	(tpALifeHumanAbstract2,m_temp_item_vector,m_tpItems2);

    if
(!bfCheckIfCanNullTradersBalance(tpALifeHumanAbstract1,tpALifeHumanAbstract2,tpALifeHumanAbstract1->children.size() -
l_iItemCount1,tpALifeHumanAbstract2->children.size() - l_iItemCount2,ifComputeBalance(tpALifeHumanAbstract1,m_tpItems2)
- ifComputeBalance(tpALifeHumanAbstract2,m_tpItems1))) {
        vfRestoreItems	(tpALifeHumanAbstract1,m_tpItems1);
        vfRestoreItems	(tpALifeHumanAbstract2,m_tpItems2);
    }

//	VERIFY				(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY				(tpALifeHumanAbstract2->check_inventory_consistency());

#ifdef FAST_OWNERSHIP
    vfAttachGatheredItems(tpALifeHumanAbstract1,tpALifeHumanAbstract2,m_tpBlockedItems1);
//	VERIFY					(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY					(tpALifeHumanAbstract2->check_inventory_consistency());

    vfAttachGatheredItems(tpALifeHumanAbstract2,tpALifeHumanAbstract1,m_tpBlockedItems2);
//	VERIFY					(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY					(tpALifeHumanAbstract2->check_inventory_consistency());
#else
    else {
        vfAttachGatheredItems(tpALifeHumanAbstract1,m_tpBlockedItems1);
        vfAttachGatheredItems(tpALifeHumanAbstract2,m_tpBlockedItems2);
    }
#endif
//	VERIFY					(tpALifeHumanAbstract1->check_inventory_consistency());
//	VERIFY					(tpALifeHumanAbstract2->check_inventory_consistency());

#ifdef DEBUG
    if (psAI_Flags.test(aiALife)) {
        vfPrintItems		(tpALifeHumanAbstract1);
        vfPrintItems		(tpALifeHumanAbstract2);
    }
#endif

    tpALifeHumanAbstract1->brain().m_dwTotalMoney = u32(-1);
    tpALifeHumanAbstract2->brain().m_dwTotalMoney = u32(-1);
}

void CALifeCommunicationManager::vfPerformCommunication()
{
    SCHEDULE_P_IT		I = m_tpaCombatGroups[0].begin();
    SCHEDULE_P_IT		E = m_tpaCombatGroups[0].end();
    for ( ; I != E; ++I) {
        SCHEDULE_P_IT	i = m_tpaCombatGroups[1].begin();
        SCHEDULE_P_IT	e = m_tpaCombatGroups[1].end();
        for ( ; i != e; ++i) {
            if (!(*I)->base()->children.empty() || !(*i)->base()->children.empty())
                vfPerformTrading(smart_cast<CSE_ALifeHumanAbstract*>(*I),smart_cast<CSE_ALifeHumanAbstract*>(*i));
        }
    }
}

void CALifeCommunicationManager::communicate_with_customer(CSE_ALifeHumanAbstract *tpALifeHumanAbstract, CSE_ALifeTrader
*tpALifeTrader)
{
    // process group of stalkers
    CSE_ALifeGroupAbstract					*l_tpALifeAbstractGroup =
smart_cast<CSE_ALifeGroupAbstract*>(tpALifeHumanAbstract);
    if (l_tpALifeAbstractGroup) {
        OBJECT_IT							I = l_tpALifeAbstractGroup->m_tpMembers.begin();
        OBJECT_IT							E = l_tpALifeAbstractGroup->m_tpMembers.end();
        for ( ; I != E; ++I)
            communicate_with_customer		(smart_cast<CSE_ALifeHumanAbstract*>(objects().object(*I)),tpALifeTrader);
        return;
    }

    // trade items
#ifdef DEBUG
    if (psAI_Flags.test(aiALife)) {
        Msg									("Selling all the items to %s",tpALifeTrader->name_replace());
    }
#endif
    CSE_ALifeItemPDA						*original_pda = 0;
    tpALifeHumanAbstract->brain().m_dwTotalMoney	= tpALifeHumanAbstract->m_dwMoney;
    {
        OBJECT_IT							I = tpALifeHumanAbstract->children.begin();
        OBJECT_IT							E = tpALifeHumanAbstract->children.end();
        for ( ; I != E; ++I) {
            CSE_ALifeInventoryItem			*l_tpALifeInventoryItem =
smart_cast<CSE_ALifeInventoryItem*>(objects().object(*I));
            CSE_ALifeItemPDA				*pda = smart_cast<CSE_ALifeItemPDA*>(l_tpALifeInventoryItem);
            if (pda && (pda->m_original_owner == tpALifeHumanAbstract->ID)) {
                VERIFY						(!original_pda);
                original_pda				= pda;
            }
            tpALifeHumanAbstract->detach	(l_tpALifeInventoryItem,0,true,false);
            tpALifeTrader->attach			(l_tpALifeInventoryItem,true);
            u32								l_dwItemCost = tpALifeTrader->dwfGetItemCost(l_tpALifeInventoryItem);
            tpALifeHumanAbstract->brain().m_dwTotalMoney += l_dwItemCost;
            tpALifeTrader->m_dwMoney		-= l_dwItemCost;
        }
        tpALifeHumanAbstract->children.clear();
    }

    sort									(tpALifeTrader->children.begin(),tpALifeTrader->children.end());

    tpALifeHumanAbstract->m_dwMoney			= tpALifeHumanAbstract->brain().m_dwTotalMoney;

    m_temp_item_vector.clear				();
    append_item_vector						(tpALifeTrader->children,m_temp_item_vector);

    m_tpBlockedItems1.clear					();
    for (int i=0; i<8; ++i) {
        int									l_iItemCount = 0;
        vfRunFunctionByIndex				(tpALifeHumanAbstract,m_tpBlockedItems1,m_temp_item_vector,i,l_iItemCount);
        if (l_iItemCount) {
            vfAssignItemParents				(tpALifeHumanAbstract,l_iItemCount);
            ITEM_P_IT						I = m_temp_item_vector.begin();
            ITEM_P_IT						E = m_temp_item_vector.end();
            for ( ; I != E; ++I) {
                if (!(*I)->attached())
                    continue;
                OBJECT_IT					J =
std::lower_bound(tpALifeTrader->children.begin(),tpALifeTrader->children.end(),(*I)->base()->ID);
                R_ASSERT					((tpALifeTrader->children.end() != J) && (*J == (*I)->base()->ID) && (((J+1)
== tpALifeTrader->children.end()) || (*(J + 1) != (*I)->base()->ID)));
                tpALifeTrader->children.erase(J);
            }
            I								=
remove_if(m_temp_item_vector.begin(),m_temp_item_vector.end(),CRemoveAttachedItemsPredicate());
            m_temp_item_vector.erase		(I,m_temp_item_vector.end());
        }
    }

    tpALifeTrader->m_dwMoney				+= tpALifeHumanAbstract->m_dwMoney -
tpALifeHumanAbstract->brain().m_dwTotalMoney;
    tpALifeHumanAbstract->m_dwMoney			= tpALifeHumanAbstract->brain().m_dwTotalMoney;
    tpALifeHumanAbstract->brain().m_dwTotalMoney	= u32(-1);

    R_ASSERT2								(int(tpALifeTrader->m_dwMoney) >= 0,"Trader must have enough money to pay
for the artefacts!");

#ifdef DEBUG
    if (psAI_Flags.test(aiALife)) {
        Msg									("Assigning correct parents");
    }
#endif
#ifdef FAST_OWNERSHIP
    vfAttachGatheredItems					(tpALifeHumanAbstract,tpALifeTrader,m_tpBlockedItems1);
    vfAttachGatheredItems					(tpALifeTrader,tpALifeHumanAbstract,m_tpBlockedItems2);
#else
    vfAttachGatheredItems					(tpALifeHumanAbstract,m_tpBlockedItems1);
    vfAttachGatheredItems					(tpALifeTrader,m_tpBlockedItems2);
#endif

    {
        OBJECT_IT							I =
std::find(tpALifeTrader->children.begin(),tpALifeTrader->children.end(),original_pda->ID);
        VERIFY								(I != tpALifeTrader->children.end());
        tpALifeTrader->detach				(original_pda);
        tpALifeHumanAbstract->attach		(original_pda,true);
    }
}
/**/
