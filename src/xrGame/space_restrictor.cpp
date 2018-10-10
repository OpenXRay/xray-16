////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restrictor.cpp
//	Created 	: 17.08.2004
//  Modified 	: 17.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Space restrictor
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "space_restrictor.h"
#include "xrServer_Objects_ALife.h"
#include "Level.h"
#include "space_restriction_manager.h"
#include "restriction_space.h"
#include "ai_space.h"
#include "CustomZone.h"
#include "xrEngine/xr_collide_form.h"
#include "xrScriptEngine/Functor.hpp"
#include "xrEngine/GameFont.h"
#ifdef DEBUG
#include "debug_renderer.h"
#endif

//Alundaio
#include "RadioactiveZone.h"
BOOL g_ai_die_in_anomaly = 0;
//-Alundaio 

CSpaceRestrictor::~CSpaceRestrictor() {}
void CSpaceRestrictor::Center(Fvector& C) const { XFORM().transform_tiny(C, GetCForm()->getSphere().P); }
float CSpaceRestrictor::Radius() const { return (GetCForm()->getRadius()); }
BOOL CSpaceRestrictor::net_Spawn(CSE_Abstract* data)
{
    actual(false);

    CSE_Abstract* abstract = (CSE_Abstract*)data;
    CSE_ALifeSpaceRestrictor* se_shape = smart_cast<CSE_ALifeSpaceRestrictor*>(abstract);
    R_ASSERT(se_shape);

    m_space_restrictor_type = se_shape->m_space_restrictor_type;

    CCF_Shape* shape = new CCF_Shape(this);
    SetCForm(shape);

    for (u32 i = 0; i < se_shape->shapes.size(); ++i)
    {
        CShapeData::shape_def& S = se_shape->shapes[i];
        switch (S.type)
        {
        case 0:
        {
            shape->add_sphere(S.data.sphere);
            break;
        }
        case 1:
        {
            shape->add_box(S.data.box);
            break;
        }
        }
    }

    shape->ComputeBounds();

    BOOL result = inherited::net_Spawn(data);

    if (!result)
        return (FALSE);

    CCustomZone* zone = smart_cast<CCustomZone*>(this);
    if (g_ai_die_in_anomaly == 0 || !zone || smart_cast<CRadioactiveZone*>(zone))
        spatial.type &= ~STYPE_VISIBLEFORAI;

    setEnabled(FALSE);
    setVisible(FALSE);

    if (!ai().get_level_graph() || (RestrictionSpace::ERestrictorTypes(se_shape->m_space_restrictor_type) ==
                                       RestrictionSpace::eRestrictorTypeNone))
        return (TRUE);

    Level().space_restriction_manager().register_restrictor(
        this, RestrictionSpace::ERestrictorTypes(se_shape->m_space_restrictor_type));

    return (TRUE);
}

void CSpaceRestrictor::net_Destroy()
{
    inherited::net_Destroy();

    if (!ai().get_level_graph())
        return;

    if (RestrictionSpace::ERestrictorTypes(m_space_restrictor_type) == RestrictionSpace::eRestrictorTypeNone)
        return;

    Level().space_restriction_manager().unregister_restrictor(this);
}

bool CSpaceRestrictor::inside(const Fsphere& sphere) const
{
    if (!actual())
        prepare();

    if (!m_selfbounds.intersect(sphere))
        return (false);

    return (prepared_inside(sphere));
}

BOOL CSpaceRestrictor::UsedAI_Locations() { return (FALSE); }
void CSpaceRestrictor::spatial_move()
{
    inherited::spatial_move();
    actual(false);
}

void CSpaceRestrictor::prepare() const
{
    Center(m_selfbounds.P);
    m_selfbounds.R = Radius();

    m_spheres.resize(0);
    m_boxes.resize(0);

    const CCF_Shape* shape = (const CCF_Shape*)GetCForm();

    typedef xr_vector<CCF_Shape::shape_def> SHAPES;

    SHAPES::const_iterator I = shape->shapes.begin();
    SHAPES::const_iterator E = shape->shapes.end();
    for (; I != E; ++I)
    {
        switch ((*I).type)
        {
        case 0:
        { // sphere
            Fsphere temp;
            const Fsphere& sphere = (*I).data.sphere;
            XFORM().transform_tiny(temp.P, sphere.P);
            temp.R = sphere.R;
            m_spheres.push_back(temp);
            break;
        }
        case 1:
        { // box
            Fmatrix sphere;
            const Fmatrix& box = (*I).data.box;
            sphere.mul_43(XFORM(), box);

            // Build points
            Fvector A, B[8];
            CPlanes temp;
            A.set(-.5f, -.5f, -.5f);
            sphere.transform_tiny(B[0], A);
            A.set(-.5f, -.5f, +.5f);
            sphere.transform_tiny(B[1], A);
            A.set(-.5f, +.5f, +.5f);
            sphere.transform_tiny(B[2], A);
            A.set(-.5f, +.5f, -.5f);
            sphere.transform_tiny(B[3], A);
            A.set(+.5f, +.5f, +.5f);
            sphere.transform_tiny(B[4], A);
            A.set(+.5f, +.5f, -.5f);
            sphere.transform_tiny(B[5], A);
            A.set(+.5f, -.5f, +.5f);
            sphere.transform_tiny(B[6], A);
            A.set(+.5f, -.5f, -.5f);
            sphere.transform_tiny(B[7], A);

            temp.m_planes[0].build(B[0], B[3], B[5]);
            temp.m_planes[1].build(B[1], B[2], B[3]);
            temp.m_planes[2].build(B[6], B[5], B[4]);
            temp.m_planes[3].build(B[4], B[2], B[1]);
            temp.m_planes[4].build(B[3], B[2], B[4]);
            temp.m_planes[5].build(B[1], B[0], B[6]);

            m_boxes.push_back(temp);

            break;
        }
        default: NODEFAULT;
        }
    }

    actual(true);
}

bool CSpaceRestrictor::prepared_inside(const Fsphere& sphere) const
{
    VERIFY(actual());

    {
        SPHERES::const_iterator I = m_spheres.begin();
        SPHERES::const_iterator E = m_spheres.end();
        for (; I != E; ++I)
            if (sphere.intersect(*I))
                return (true);
    }

    {
        BOXES::const_iterator I = m_boxes.begin();
        BOXES::const_iterator E = m_boxes.end();
        for (; I != E; ++I)
        {
            for (u32 i = 0; i < PLANE_COUNT; ++i)
                if ((*I).m_planes[i].classify(sphere.P) > sphere.R)
                    goto continue_loop;
            return (true);
        continue_loop:
            continue;
        }
    }
    return (false);
}

#ifdef DEBUG

#include "CustomZone.h"
#include "xrUICore/ui_base.h"

extern Flags32 dbg_net_Draw_Flags;

void CSpaceRestrictor::OnRender()
{
    if (!bDebug)
        return;
    if (!(dbg_net_Draw_Flags.is_any(dbg_draw_customzone)))
        return;
    // RCache.OnFrameEnd();
    GEnv.DRender->OnFrameEnd();
    Fvector l_half;
    l_half.set(.5f, .5f, .5f);
    Fmatrix l_ball, l_box;
    xr_vector<CCF_Shape::shape_def>& l_shapes = ((CCF_Shape*)GetCForm())->Shapes();
    xr_vector<CCF_Shape::shape_def>::iterator l_pShape;

    u32 Color = 0;
    CCustomZone* custom_zone = smart_cast<CCustomZone*>(this);
    if (custom_zone && custom_zone->IsEnabled())
        Color = color_xrgb(0, 255, 255);
    else
        Color = color_xrgb(255, 0, 0);

    for (l_pShape = l_shapes.begin(); l_shapes.end() != l_pShape; ++l_pShape)
    {
        switch (l_pShape->type)
        {
        case 0:
        {
            Fsphere& l_sphere = l_pShape->data.sphere;
            l_ball.scale(l_sphere.R, l_sphere.R, l_sphere.R);
            // l_ball.scale(1.f, 1.f, 1.f);
            Fvector l_p;
            XFORM().transform(l_p, l_sphere.P);
            l_ball.translate_add(l_p);
            // l_ball.mul(XFORM(), l_ball);
            // l_ball.mul(l_ball, XFORM());
            Level().debug_renderer().draw_ellipse(l_ball, Color);
        }
        break;
        case 1:
        {
            l_box.mul(XFORM(), l_pShape->data.box);
            Level().debug_renderer().draw_obb(l_box, l_half, Color);
        }
        break;
        }
    }

    if (Level().CurrentViewEntity()->Position().distance_to(XFORM().c) < 100.0f)
    {
        // DRAW name

        Fmatrix res;
        res.mul(Device.mFullTransform, XFORM());

        Fvector4 v_res;

        float delta_height = 0.f;

        // get up on 2 meters
        Fvector shift;
        static float gx = 0.0f;
        static float gy = 2.0f;
        static float gz = 0.0f;
        shift.set(gx, gy, gz);
        res.transform(v_res, shift);

        // check if the object in sight
        if (v_res.z < 0 || v_res.w < 0)
            return;
        if (v_res.x < -1.f || v_res.x > 1.f || v_res.y < -1.f || v_res.y > 1.f)
            return;

        // get real (x,y)
        float x = (1.f + v_res.x) / 2.f * (Device.dwWidth);
        float y = (1.f - v_res.y) / 2.f * (Device.dwHeight) - delta_height;

        UI().Font().pFontMedium->SetColor(0xffff0000);
        UI().Font().pFontMedium->OutSet(x, y -= delta_height);
        UI().Font().pFontMedium->OutNext(Name());
        CCustomZone* z = smart_cast<CCustomZone*>(this);
        if (z)
        {
            string64 str;
            switch (z->ZoneState())
            {
            case CCustomZone::eZoneStateIdle: xr_strcpy(str, "IDLE"); break;
            case CCustomZone::eZoneStateAwaking: xr_strcpy(str, "AWAKING"); break;
            case CCustomZone::eZoneStateBlowout: xr_strcpy(str, "BLOWOUT"); break;
            case CCustomZone::eZoneStateAccumulate: xr_strcpy(str, "ACCUMULATE"); break;
            case CCustomZone::eZoneStateDisabled: xr_strcpy(str, "DISABLED"); break;
            };
            UI().Font().pFontMedium->OutNext(str);
        }
    }
}
#endif
