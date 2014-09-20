//----------------------------------------------------
// file: DetailObjects.h
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ESceneDOTools.h"
#include "Scene.h"
#include "cl_intersect.h"
#include "bottombar.h"
#include "../ECore/Editor/ui_main.h"

int EDetailManager::RaySelect(int flag, float& dist, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly)
{
// box selected only

	if (!m_Flags.is(flSlotBoxesDraw)) return 0;

	float			fx,fz;
    Fbox			bbox;
    Fvector 		P;
    int 			sx=-1, sz=-1;

    int count = 0;

    for (u32 z=0; z<dtH.size_z; z++){
        fz			= fromSlotZ(z);
        for (u32 x=0; x<dtH.size_x; x++){
			DetailSlot* slot= dtSlots+z*dtH.size_x+x;
            fx		= fromSlotX(x);
            bbox.min.set(fx-DETAIL_SLOT_SIZE_2, slot->r_ybase(), 					fz-DETAIL_SLOT_SIZE_2);
            bbox.max.set(fx+DETAIL_SLOT_SIZE_2, slot->r_ybase()+slot->r_yheight(), 	fz+DETAIL_SLOT_SIZE_2);
            if (bbox.Pick2(start,direction,P)){
            	float d = start.distance_to(P);
                if (d<dist){
                	dist = d;
                    sx=x; sz=z;
                }
            }
        }
    }
    if ((sx>=0)||(sz>=0)){
	    if (!bDistanceOnly){
            if (flag==-1)	
                m_Selected[sz*dtH.size_x+sx] = !m_Selected[sz*dtH.size_x+sx];
            else
                m_Selected[sz*dtH.size_x+sx] = (u8)flag;
            count++;
	    	UI->RedrawScene();
        }
    }
    return count;
}

int EDetailManager::FrustumSelect(int flag, const CFrustum& frustum)
{
// box selected only

	if (!m_Flags.is(flSlotBoxesDraw)) return 0;

    int count=0;

    float 			fx,fz;
    Fbox			bbox;
    for (u32 z=0; z<dtH.size_z; z++){
        fz			= fromSlotZ(z);
        for (u32 x=0; x<dtH.size_x; x++){
            DetailSlot* slot = dtSlots+z*dtH.size_x+x;
            fx			= fromSlotX(x);

            bbox.min.set(fx-DETAIL_SLOT_SIZE_2, slot->r_ybase(), 					fz-DETAIL_SLOT_SIZE_2);
            bbox.max.set(fx+DETAIL_SLOT_SIZE_2, slot->r_ybase()+slot->r_yheight(), 	fz+DETAIL_SLOT_SIZE_2);
			u32 mask	= 0xff;
            bool bRes 	= !!frustum.testAABB(bbox.data(),mask);
            if (bRes){
            	if (flag==-1)	
                	m_Selected[z*dtH.size_x+x] = !m_Selected[z*dtH.size_x+x];
                else
                	m_Selected[z*dtH.size_x+x] = (u8)flag;
                
            	count++;
            }
        }
    }
    UI->RedrawScene();
    return count;
}

void EDetailManager::SelectObjects(bool flag)
{
//	for (int i=0; i<m_Selected.size(); i++)
//    	m_Selected[i] = flag;
	for (U8It it=m_Selected.begin(); it!=m_Selected.end(); it++)
    	*it = flag;
}

void EDetailManager::InvertSelection()
{
	if (!m_Flags.is(flSlotBoxesDraw)) return;
//	for (int i=0; i<m_Selected.size(); i++)
//    	m_Selected[i] = m_Selected[i];
	for (U8It it=m_Selected.begin(); it!=m_Selected.end(); it++)
    	*it = !*it;
}

int EDetailManager::SelectionCount(bool testflag)
{
	if (!m_Flags.is(flSlotBoxesDraw)) return 0;
	int count = 0;
//	for (int i=0; i<m_Selected.size(); i++)
//    	if (m_Selected[i]==testflag) count++;
	for (U8It it=m_Selected.begin(); it!=m_Selected.end(); it++)
    	if (*it==testflag) count++;
    return count;
}

