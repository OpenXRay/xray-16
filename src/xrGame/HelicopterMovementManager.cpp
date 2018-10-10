#include "pch_script.h"
#include "helicopter.h"
#include "Level.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path.h"
#include "xrAICore/Navigation/PatrolPath/patrol_path_storage.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"

SHeliMovementState::~SHeliMovementState() {}
void SHeliMovementState::net_Destroy()
{
    if (need_to_del_path && currPatrolPath)
    {
        CPatrolPath* tmp = const_cast<CPatrolPath*>(currPatrolPath);
        xr_delete(tmp);
    }
}

void SHeliMovementState::Load(LPCSTR section)
{
    float angularSpeedPitch = pSettings->r_float(section, "path_angular_sp_pitch");
    AngSP = angularSpeedPitch;
    float angularSpeedHeading = pSettings->r_float(section, "path_angular_sp_heading");
    AngSH = angularSpeedHeading;
    LinearAcc_fw = pSettings->r_float(section, "path_linear_acc_fw");
    LinearAcc_bk = pSettings->r_float(section, "path_linear_acc_bk");
    if (pSettings->line_exist(section, "flag_by_new_acc"))
        isAdnAcc = pSettings->r_float(section, "flag_by_new_acc");
    else
        isAdnAcc = 0;
    onPointRangeDist = pSettings->r_float(section, "on_point_range_dist");
    maxLinearSpeed = pSettings->r_float(section, "velocity");
    min_altitude = pSettings->r_float(section, "min_altitude");

    float y0 = pSettings->r_float(section, "path_angular_sp_pitch_0");
    PitchSpB = y0;
    PitchSpK = (angularSpeedPitch - PitchSpB) / maxLinearSpeed;

    y0 = pSettings->r_float(section, "path_angular_sp_heading_0");
    HeadingSpB = y0;
    HeadingSpK = (angularSpeedHeading - HeadingSpB) / maxLinearSpeed;
    safe_altitude_add = pSettings->r_float(section, "safe_altitude");
}

float SHeliMovementState::GetAngSpeedPitch(float speed)
{
    return PitchSpK * speed + PitchSpB;
    // PitchSpK*speed+PitchSpB;
    //(AngSP - PitchSpB)*speed*AngSP + PitchSpB;
    // PitchSpB/(1 + speed * AngSP); //+-0.10
}

float SHeliMovementState::GetAngSpeedHeading(float speed)
{
    if (isAdnAcc == 0)
        return HeadingSpK * speed + HeadingSpB;

    else
        return AngSH / (2 * HeadingSpB - AngSH + speed * (HeadingSpB - AngSH) / 2);
    // HeadingSpK*speed+HeadingSpB; old veersion
}

void SHeliMovementState::Update()
{
    switch (type)
    {
    case eMovNone: break;
    case eMovToPoint: UpdateMovToPoint(); break;
    case eMovPatrolPath:
    case eMovRoundPath: UpdatePatrolPath(); break;
    case eMovLanding:
    case eMovTakeOff: break;
    default: NODEFAULT;
    };
}

void SHeliMovementState::reinit()
{
    type = eMovNone;
    currPatrolPath = NULL;
    currPatrolVertex = NULL;
    patrol_begin_idx = 0;
    patrol_path_name = "";
    need_to_del_path = false;
    curLinearSpeed = 0.0f;
    curLinearAcc = 0.0f;
    round_center.set(0.0f, 0.0f, 0.0f);
    round_radius = 0.0f;
    round_reverse = false;
    desiredPoint = parent->XFORM().c;
    currP = desiredPoint;
    float bbb;
    parent->XFORM().getHPB(currPathH, currPathP, bbb);

    speedInDestPoint = 0.0f;
}
float SHeliMovementState::GetDistanceToDestPosition() { return desiredPoint.distance_to(currP); }
void SHeliMovementState::UpdatePatrolPath()
{
    if (AlreadyOnPoint())
    {
        float dist = GetDistanceToDestPosition();
        parent->callback(GameObject::eHelicopterOnPoint)(
            dist, currP, currPatrolVertex ? currPatrolVertex->vertex_id() : -1);
        CPatrolPath::const_iterator b, e;
        currPatrolPath->begin(currPatrolVertex, b, e);
        if (b != e)
        {
            if (need_to_del_path &&
                currPatrolVertex->data().flags()) // fake flags that signals entrypoint for round path
                SetPointFlags(currPatrolVertex->vertex_id(), 0);

            currPatrolVertex = currPatrolPath->vertex((*b).vertex_id());

            Fvector p = currPatrolVertex->data().position();
            desiredPoint = p;
        }
        else
        {
            type = eMovNone;
            //			curLinearSpeed	= 0.0f;
            //			curLinearAcc	= 0.0f;
        }
    }
}

void SHeliMovementState::UpdateMovToPoint()
{
    if (AlreadyOnPoint())
    {
        float dist = GetDistanceToDestPosition();
        parent->callback(GameObject::eHelicopterOnPoint)(dist, currP, -1);
        type = eMovNone;
    }
}
extern float STEP;
bool SHeliMovementState::AlreadyOnPoint()
{
    float dist = GetDistanceToDestPosition();
    bool res = false;
    if (dist <= 0.1f)
        res = true;

    if (dist < onPointRangeDist)
    {
        Fvector P1 = currP;
        Fvector dir;
        dir.setHP(currPathH, 0.0f);
        P1.mad(dir, curLinearSpeed * STEP);
        float new_dist = desiredPoint.distance_to(P1);
        res = new_dist > dist;
    }
    //	if(res)
    //		Msg("--------OnPoint id=[%d] dist=[%f]", currPatrolVertex->vertex_id(), dist);

    return res;
}

void SHeliMovementState::getPathAltitude(Fvector& point, float base_altitude)
{
    Fbox boundingVolume = Level().ObjectSpace.GetBoundingVolume();
    Fvector boundSize;
    boundingVolume.getsize(boundSize);

    collide::rq_result cR;
    Fvector down_dir;
    down_dir.set(0.0f, -1.0f, 0.0f);

    point.y = boundingVolume.vMax.y + EPS_L;
    VERIFY(_valid(point));

    Level().ObjectSpace.RayPick(point, down_dir, boundSize.y + 1.0f, collide::rqtStatic, cR, NULL);

    point.y = point.y - cR.range;

    if (point.y + base_altitude < boundingVolume.vMax.y)
        point.y += base_altitude;
    else
        point.y = boundingVolume.vMax.y - EPS_L;

    VERIFY(_valid(point));

    float minY = boundingVolume.vMin.y; //+(m_boundingVolume.vMax.y-m_boundingVolume.vMin.y)*m_heli->m_data.m_alt_korridor;
    float maxY = boundingVolume.vMax.y + base_altitude;
    clamp(point.y, minY, maxY);
    VERIFY(_valid(point));
}
void SHeliMovementState::SetDestPosition(Fvector* pos)
{
    desiredPoint = *pos;
    type = eMovToPoint;

    if (need_to_del_path && currPatrolPath)
    {
        CPatrolPath* tmp = const_cast<CPatrolPath*>(currPatrolPath);
        xr_delete(tmp);
        need_to_del_path = false;
    }
}

void SHeliMovementState::goPatrolByPatrolPath(LPCSTR path_name, int start_idx)
{
    if (need_to_del_path && currPatrolPath)
    {
        CPatrolPath* tmp = const_cast<CPatrolPath*>(currPatrolPath);
        xr_delete(tmp);
    }

    patrol_begin_idx = start_idx;
    patrol_path_name = path_name;

    currPatrolPath = ai().patrol_paths().path(patrol_path_name);
    need_to_del_path = false;
    currPatrolVertex = currPatrolPath->vertex(patrol_begin_idx);

    desiredPoint = currPatrolVertex->data().position();

    type = eMovPatrolPath;
}

void SHeliMovementState::save(NET_Packet& output_packet)
{
    output_packet.w_s16((s16)type);
    output_packet.w_u32(patrol_begin_idx);
    output_packet.w_stringZ(patrol_path_name);

    output_packet.w_float(maxLinearSpeed);
    output_packet.w_float(LinearAcc_fw);
    output_packet.w_float(LinearAcc_bk);

    output_packet.w_float(speedInDestPoint);

    output_packet.w_vec3(desiredPoint);

    output_packet.w_float(curLinearSpeed);
    output_packet.w_float(curLinearAcc);

    output_packet.w_vec3(currP);

    output_packet.w_float(currPathH);
    output_packet.w_float(currPathP);

    output_packet.w_vec3(round_center);

    output_packet.w_float(round_radius);

    output_packet.w_u8(round_reverse ? 1 : 0);

    output_packet.w_float(onPointRangeDist);

    if (type == eMovPatrolPath)
    {
        output_packet.w_s32(currPatrolVertex->vertex_id());
    }
}

void SHeliMovementState::load(IReader& input_packet)
{
    type = (EHeilMovementState)input_packet.r_s16();
    patrol_begin_idx = input_packet.r_u32();
    input_packet.r_stringZ(patrol_path_name);

    maxLinearSpeed = input_packet.r_float();
    LinearAcc_fw = input_packet.r_float();
    LinearAcc_bk = input_packet.r_float();

    speedInDestPoint = input_packet.r_float();

    input_packet.r_fvector3(desiredPoint);

    curLinearSpeed = input_packet.r_float();
    curLinearAcc = input_packet.r_float();

    input_packet.r_fvector3(currP);

    currPathH = input_packet.r_float();
    currPathP = input_packet.r_float();

    input_packet.r_fvector3(round_center);

    round_radius = input_packet.r_float();

    round_reverse = !!input_packet.r_u8();

    onPointRangeDist = input_packet.r_float();

    if (type == eMovPatrolPath)
    {
        currPatrolPath = ai().patrol_paths().path(patrol_path_name);
        int idx = input_packet.r_s32();
        currPatrolVertex = currPatrolPath->vertex(idx);
    }
    if (type == eMovRoundPath)
    {
        goByRoundPath(round_center, round_radius, !round_reverse);
    }
}

float SHeliMovementState::GetSafeAltitude()
{
    Fbox boundingVolume = Level().ObjectSpace.GetBoundingVolume();
    return boundingVolume.vMax.y + safe_altitude_add;
}

void SHeliMovementState::CreateRoundPoints(
    Fvector center, float radius, float start_h, float end_h, xr_vector<STmpPt>& round_points)
{
    float height = center.y;

    float round_len = 2 * PI * radius;
    static float dist = 30.0f; // dist between points
    float td = 2 * PI * dist / round_len;
    float dir_h = 0.0f;

    dir_h = start_h;
    while (dir_h + td < end_h)
    {
        Fvector dir, new_pt;
        dir.setHP(dir_h, 0.0f);
        new_pt.mad(center, dir, radius);
        new_pt.y = height;
        round_points.push_back(STmpPt(new_pt, dir_h));
        dir_h += td;
    }
}

void SHeliMovementState::goByRoundPath(Fvector center_, float radius_, bool clockwise_)
{
    if (type == eMovRoundPath)
        clockwise_ = !clockwise_;

    float r_verify = maxLinearSpeed * GetAngSpeedHeading(maxLinearSpeed);
    if (r_verify > radius_)
    {
#ifndef MASTER_GOLD
        Msg("! Helicopter: cannot build round path R=%f. Min R=%f", radius_, r_verify);
#endif // #ifndef MASTER_GOLD
        return;
    }

    round_center = center_;
    round_radius = radius_;
    round_reverse = !clockwise_;

    if (need_to_del_path && currPatrolPath)
    {
        CPatrolPath* tmp = const_cast<CPatrolPath*>(currPatrolPath);
        xr_delete(tmp);
    }
    need_to_del_path = true;
    u32 pt_idx = 0;
    CPatrolPath* pp = new CPatrolPath("heli_round_path");

    float start_h = 0.0f;
    float end_h = PI_MUL_2 - EPS;

    xr_vector<STmpPt> round_points;
    xr_vector<STmpPt>::iterator it, it_e;
    CreateRoundPoints(center_, radius_, start_h, end_h, round_points);
    if (clockwise_)
        std::reverse(round_points.begin() + 1, round_points.end());

    it = round_points.begin();
    it_e = round_points.end();
    for (; it != it_e; ++it, ++pt_idx)
    {
        string128 pt_name;
        xr_sprintf(pt_name, "heli_round_path_pt_%d", pt_idx);
        CPatrolPoint pt = CPatrolPoint(
            (CLevelGraph*)0, (CGameLevelCrossTable*)0, (CGameGraph*)0, pp, (*it).point, u32(-1), 0, pt_name);
        pp->add_vertex(pt, pt_idx);
        if (pt_idx)
            pp->add_edge(pt_idx - 1, pt_idx, 1.f);
    }
    pp->add_edge(pt_idx - 1, 0, 1.f);

    currPatrolPath = pp;

    // find nearest point to start from...
    u32 start_vertex_id = 0;
    float min_dist = flt_max;
    float stop_t = curLinearSpeed / LinearAcc_bk;
    float stop_path = curLinearSpeed * stop_t - LinearAcc_bk * stop_t * stop_t / 2.0f;

    CPatrolPath::const_vertex_iterator b = currPatrolPath->vertices().begin();
    CPatrolPath::const_vertex_iterator e = currPatrolPath->vertices().end();
    for (; b != e; ++b)
    {
        float d = (*b).second->data().position().distance_to(currP);
        if ((d > stop_path) && (d < min_dist))
        {
            min_dist = d;
            start_vertex_id = (*b).first;
        }
    }

    SetPointFlags(start_vertex_id, 1);
    currPatrolVertex = currPatrolPath->vertex(start_vertex_id);
    desiredPoint = currPatrolVertex->data().position();

    type = eMovRoundPath;
}

void SHeliMovementState::SetPointFlags(u32 idx, u32 new_flags)
{
    CPatrolPath* p = const_cast<CPatrolPath*>(currPatrolPath);
    CPatrolPoint* pt_curr = &p->vertex(idx)->data();
    CPatrolPoint* pt_new = new CPatrolPoint((CLevelGraph*)0, (CGameLevelCrossTable*)0, (CGameGraph*)0, currPatrolPath,
        pt_curr->position(), u32(-1), new_flags, pt_curr->name());

    p->vertex(idx)->data(*pt_new);
    //	xr_delete(pt_curr);
}

float SHeliMovementState::GetSpeedInDestPoint()
{
    if (need_to_del_path && currPatrolVertex && currPatrolVertex->data().flags())
        return 0.0f;
    else
        return speedInDestPoint;
}
void SHeliMovementState::SetSpeedInDestPoint(float val) { speedInDestPoint = val; }
Fvector CHelicopter::GetCurrVelocityVec()
{
    Fvector dir;
    dir.setHP(m_movement.currPathH, m_movement.currPathP);
    //	dir.sub				(m_movement.desiredPoint,m_movement.currP);
    dir.normalize_safe();
    return dir;
}

#ifdef DEBUG
void CHelicopter::OnRender()
{
    /*
        if(!bDebug) return;

        if(!m_movement.currPatrolPath) return;

        CPatrolPath::const_vertex_iterator b = m_movement.currPatrolPath->vertices().begin();
        CPatrolPath::const_vertex_iterator e = m_movement.currPatrolPath->vertices().end();
        for ( ; b != e; ++b) {
            Fvector p = (*b).second->data().position();
            Level().debug_renderer().draw_aabb  (p,0.1f,0.1f,0.1f,color_xrgb(0,255,0));
        }
    */
    /*
        Fvector pos			= Level().CurrentEntity()->Position();
        static float	radius		= 50.0f;//meters
        float	round_len	= 2*PI*radius;
        static float	dist		= 10.0f;//dist between points
        float	td			= 2*PI*dist/round_len;
        float	dir_h		= 0.0f;
        xr_vector<Fvector>	round_points;

        while(dir_h+td<2*PI){
            Fvector dir, new_pt;
            dir.setHP(dir_h,0.0f);
            new_pt.mad(pos,dir,radius);
            new_pt.y += 1.0f;
            round_points.push_back(new_pt);
            dir_h	+= td;
        }

        xr_vector<Fvector>::iterator it = round_points.begin();
        xr_vector<Fvector>::iterator it_e = round_points.end();
        for(;it!=it_e;++it){
            Level().debug_renderer().draw_aabb  ((*it),0.1f,0.1f,0.1f,color_xrgb(0,255,0));
        }
    */
    /*	Level().debug_renderer().draw_line(Fidentity,m_heli->m_right_rocket_bone_xform.c,
       m_heli->m_data.m_destEnemyPos,color_xrgb(0,255,0));

        Level().debug_renderer().draw_line(Fidentity,m_heli->XFORM().c,m_heli->m_data.m_destEnemyPos,color_xrgb(255,0,0));
        return;
    */
}
#endif
