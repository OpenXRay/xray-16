////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_object.cpp
//	Created 	: 28.08.2007
//  Modified 	: 28.08.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover object class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_object.h"
#include "xrServerEntities/xrServer_Objects_Alife_Smartcovers.h"
#include "Level.h"
#include "debug_renderer.h"
#include "ai_space.h"
#include "cover_manager.h"
#include "xrAICore/Navigation/ai_object_location.h"
#include "smart_cover.h"
#include "smart_cover_description.h"
#include "smart_cover_loophole.h"
#include "xrEngine/xr_collide_form.h"
using smart_cover::object;

void object::Load(LPCSTR section)
{
    inherited::Load(section);

    m_enter_min_enemy_distance = pSettings->r_float(section, "enter_min_enemy_distance");
    m_exit_min_enemy_distance = pSettings->r_float(section, "exit_min_enemy_distance");
}

BOOL object::net_Spawn(CSE_Abstract* server_entity)
{
    CSE_SmartCover* smart_cover = smart_cast<CSE_SmartCover*>(server_entity);
    VERIFY(smart_cover);

    if (!smart_cover->m_description.size())
        Msg("! smart cover %s has no description", smart_cover->name_replace());

    CCF_Shape* shape = new CCF_Shape(this);
    SetCForm(shape);

    typedef CShapeData::ShapeVec ShapeVec;
    ShapeVec::iterator I = smart_cover->shapes.begin();
    ShapeVec::iterator E = smart_cover->shapes.end();
    for (; I != E; ++I)
    {
        switch ((*I).type)
        {
        case 0:
        {
            shape->add_sphere((*I).data.sphere);
            break;
        }
        case 1:
        {
            shape->add_box((*I).data.box);
            break;
        }
        }
    }

    shape->ComputeBounds();

    if (!inherited::net_Spawn(server_entity))
        return (FALSE);

    spatial.type &= ~STYPE_VISIBLEFORAI;

    if (ai().get_alife() && smart_cover->m_description.size())
        m_cover = ai().cover_manager().add_smart_cover(smart_cover->m_description.c_str(), *this,
            smart_cover->m_is_combat_cover ? true : false, smart_cover->m_can_fire ? true : false,
            smart_cover->m_available_loopholes);
    else
        m_cover = 0;

    processing_deactivate();

    setEnabled(FALSE);
    setVisible(FALSE);

    return (TRUE);
}

void object::Center(Fvector& result) const { XFORM().transform_tiny(result, GetCForm()->getSphere().P); }
float object::Radius() const { return (GetCForm()->getRadius()); }
void object::UpdateCL() { NODEFAULT; }
void object::shedule_Update(u32 dt) { NODEFAULT; }
#ifdef DEBUG
void dbg_draw_frustum(float FOV, float _FAR, float A, Fvector& P, Fvector& D, Fvector& U);

void object::OnRender()
{
    GEnv.DRender->OnFrameEnd();
    Fvector l_half;
    l_half.set(.5f, .5f, .5f);
    Fmatrix l_ball, l_box;
    u32 Color = color_xrgb(0, 255, 0);

    typedef xr_vector<CCF_Shape::shape_def> Shapes;
    Shapes& l_shapes = ((CCF_Shape*)GetCForm())->Shapes();
    Shapes::iterator l_pShape;
    CDebugRenderer& renderer = Level().debug_renderer();
    for (l_pShape = l_shapes.begin(); l_shapes.end() != l_pShape; ++l_pShape)
    {
        switch (l_pShape->type)
        {
        case 0:
        {
            Fsphere& l_sphere = l_pShape->data.sphere;
            l_ball.scale(l_sphere.R, l_sphere.R, l_sphere.R);
            Fvector l_p;
            XFORM().transform(l_p, l_sphere.P);
            l_ball.translate_add(l_p);
            renderer.draw_ellipse(l_ball, Color);
            break;
        }
        case 1:
        {
            l_box.mul(XFORM(), l_pShape->data.box);
            renderer.draw_obb(l_box, l_half, Color);
            break;
        }
        }
    }

    if (!m_cover)
        return;

    typedef smart_cover::description::Loopholes::const_iterator const_iterator;
    const_iterator I = m_cover->loopholes().begin();
    const_iterator E = m_cover->loopholes().end();
    for (; I != E; ++I)
    {
        smart_cover::loophole* loophole = *I;
        Fvector position = m_cover->fov_position(*loophole);
        Fvector direction = m_cover->fov_direction(*loophole);
        Fvector up = XFORM().j;
        dbg_draw_frustum(loophole->fov() * 180.f / PI, loophole->range(), 1.f, position, direction, up);
    }
}
#endif // DEBUG

bool object::inside(Fvector const& position) const
{
    CCF_Shape* shape = static_cast<CCF_Shape*>(GetCForm());
    VERIFY(shape);

    typedef xr_vector<CCF_Shape::shape_def> Shapes;
    Shapes::const_iterator i = shape->shapes.begin();
    Shapes::const_iterator e = shape->shapes.end();
    for (; i != e; ++i)
    {
        switch ((*i).type)
        {
        case 0:
        {
            if ((*i).data.sphere.P.distance_to(position) <= (*i).data.sphere.R)
                return (true);

            continue;
        }
        case 1:
        {
            Fmatrix matrix;
            const Fmatrix& box = (*i).data.box;
            matrix.mul_43(XFORM(), box);
            Fvector A, B[8];
            Fplane plane;
            A.set(-.5f, -.5f, -.5f);
            matrix.transform_tiny(B[0], A);
            A.set(-.5f, -.5f, +.5f);
            matrix.transform_tiny(B[1], A);
            A.set(-.5f, +.5f, +.5f);
            matrix.transform_tiny(B[2], A);
            A.set(-.5f, +.5f, -.5f);
            matrix.transform_tiny(B[3], A);
            A.set(+.5f, +.5f, +.5f);
            matrix.transform_tiny(B[4], A);
            A.set(+.5f, +.5f, -.5f);
            matrix.transform_tiny(B[5], A);
            A.set(+.5f, -.5f, +.5f);
            matrix.transform_tiny(B[6], A);
            A.set(+.5f, -.5f, -.5f);
            matrix.transform_tiny(B[7], A);

            plane.build(B[0], B[3], B[5]);
            if (plane.classify(position) <= 0.f)
                return (true);
            plane.build(B[1], B[2], B[3]);
            if (plane.classify(position) <= 0.f)
                return (true);
            plane.build(B[6], B[5], B[4]);
            if (plane.classify(position) <= 0.f)
                return (true);
            plane.build(B[4], B[2], B[1]);
            if (plane.classify(position) <= 0.f)
                return (true);
            plane.build(B[3], B[2], B[4]);
            if (plane.classify(position) <= 0.f)
                return (true);
            plane.build(B[1], B[0], B[6]);
            if (plane.classify(position) <= 0.f)
                return (true);

            continue;
        }
        default: NODEFAULT;
        }
    }

    return (false);
}
