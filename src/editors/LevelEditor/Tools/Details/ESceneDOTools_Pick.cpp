#include "stdafx.h"

int EDetailManager::RaySelect(int flag, float &dist, const Fvector &start, const Fvector &direction, BOOL bDistanceOnly)
{
    // box selected only

    if (!m_Flags.is(flSlotBoxesDraw))
        return 0;

    float fx, fz;
    Fbox bbox;
    Fvector P;
    int sx = -1, sz = -1;

    int count = 0;

    for (u32 z = 0; z < dtH.z_size(); z++)
    {
        fz = fromSlotZ(z);
        for (u32 x = 0; x < dtH.x_size(); x++)
        {
            DetailSlot* slot = dtSlots + z * dtH.z_size() + x;
            fx = fromSlotX(x);
            bbox.vMin.set(fx - DETAIL_SLOT_SIZE_2, slot->r_ybase(), fz - DETAIL_SLOT_SIZE_2);
            bbox.vMax.set(fx + DETAIL_SLOT_SIZE_2, slot->r_ybase() + slot->r_yheight(), fz + DETAIL_SLOT_SIZE_2);
            if (bbox.Pick2(start, direction, P))
            {
                float d = start.distance_to(P);
                if (d < dist)
                {
                    dist = d;
                    sx = x;
                    sz = z;
                }
            }
        }
    }
    if ((sx >= 0) || (sz >= 0))
    {
        if (!bDistanceOnly)
        {
            if (flag == -1)
                m_Selected[sz * dtH.x_size() + sx] = !m_Selected[sz * dtH.x_size() + sx];
            else
                m_Selected[sz * dtH.x_size() + sx] = (u8)flag;
            count++;
            UI->RedrawScene();
        }
    }
    return count;
}

int EDetailManager::FrustumSelect(int flag, const CFrustum &frustum)
{
    // box selected only

    if (!m_Flags.is(flSlotBoxesDraw))
        return 0;

    int count = 0;

    float fx, fz;
    Fbox bbox;
    for (u32 z = 0; z < dtH.z_size(); z++)
    {
        fz = fromSlotZ(z);
        for (u32 x = 0; x < dtH.x_size(); x++)
        {
            DetailSlot *slot = dtSlots + z * dtH.x_size() + x;
            fx = fromSlotX(x);

            bbox.vMin.set(fx - DETAIL_SLOT_SIZE_2, slot->r_ybase(), fz - DETAIL_SLOT_SIZE_2);
            bbox.vMax.set(fx + DETAIL_SLOT_SIZE_2, slot->r_ybase() + slot->r_yheight(), fz + DETAIL_SLOT_SIZE_2);
            u32 mask = 0xff;
            bool bRes = !!frustum.testAABB(bbox.data(), mask);
            if (bRes)
            {
                if (flag == -1)
                    m_Selected[z * dtH.x_size() + x] = !m_Selected[z * dtH.x_size() + x];
                else
                    m_Selected[z * dtH.x_size() + x] = (u8)flag;

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
    for (U8It it = m_Selected.begin(); it != m_Selected.end(); it++)
        *it = flag;
}

void EDetailManager::InvertSelection()
{
    if (!m_Flags.is(flSlotBoxesDraw))
        return;
    //	for (int i=0; i<m_Selected.size(); i++)
    //    	m_Selected[i] = m_Selected[i];
    for (U8It it = m_Selected.begin(); it != m_Selected.end(); it++)
        *it = !*it;
}

int EDetailManager::SelectionCount(bool testflag)
{
    if (!m_Flags.is(flSlotBoxesDraw))
        return 0;
    int count = 0;
    //	for (int i=0; i<m_Selected.size(); i++)
    //    	if (m_Selected[i]==testflag) count++;
    for (U8It it = m_Selected.begin(); it != m_Selected.end(); it++)
        if (!!*it == testflag)
            count++;
    return count;
}
