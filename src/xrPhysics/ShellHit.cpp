#include "StdAfx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHShellSplitter.h"
#include "PHFracture.h"
#include "PHJointDestroyInfo.h"
#include "ExtendedGeom.h"

#include "PHElement.h"
#include "PHShell.h"

void CPHShell::applyHit(const Fvector& pos, const Fvector& dir, float val, const u16 id, ALife::EHitType hit_type)
{
    if (id == u16(-1))
        return;
    if (fis_zero(val) || val <= 0.001f)
        return;
    if (hit_type == ALife::eHitTypeBurn || hit_type == ALife::eHitTypeLightBurn ||
        hit_type == ALife::eHitTypeShock || hit_type == ALife::eHitTypeRadiation ||
        hit_type == ALife::eHitTypeTelepatic || hit_type == ALife::eHitTypeChemicalBurn)
        return;

#pragma todo("Kosya to kosya:this code shold treat all hit types")
    if (!m_pKinematics)
    {
        applyImpulseTrace(pos, dir, val);
        return;
    }
    switch (hit_type)
    {
    case ALife::eHitTypeExplosion: ExplosionHit(pos, dir, val, id); break;
    default: applyImpulseTrace(pos, dir, val, id);
    }
}

void CPHShell::ExplosionHit(const Fvector& pos, const Fvector& dir, float val, const u16 id)
{
    if (!isActive())
        return;
    EnableObject(0);

    auto i = elements.begin(), e = elements.end();
    float impulse = val / _sqrt(_sqrt((float)elements.size()));
    for (; i != e; ++i)
    {
        CPHElement* element = (*i);
        u16 gn = element->CPHGeometryOwner::numberOfGeoms();
        float g_impulse = impulse / gn;
        for (u16 j = 0; j < gn; ++j)
        {
            Fvector r_dir, r_pos, r_box;
            float rad = element->getRadius();
            r_box.set(rad, rad, rad);
            r_pos.random_point(r_box);
            r_dir.random_dir();
            if (!fis_zero(pos.magnitude(), EPS_L))
            {
                r_dir.mul(0.5f);
                r_dir.add(dir);
            }

            r_dir.normalize_safe();
            element->applyImpulseTrace(r_pos, r_dir, g_impulse, element->CPHGeometryOwner::Geom(j)->bone_id());
        }
    }
}
