////////////////////////////////////////////////////////////////////////////
//  Module      : xrServer_Objects_ALife_Smartcovers.cpp
//  Created     : 17.12.2008
//  Modified    :
//  Author      : Alexander Plichko
//  Description : Server objects smartcovers for ALife simulator
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "xrServer_Objects_Alife_Smartcovers.h"

#ifndef AI_COMPILER
#include "character_info.h"
#endif

#ifdef XRSE_FACTORY_EXPORTS
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#pragma warning(push)
#pragma warning(disable : 4995)
#include <luabind/luabind.hpp>
#include <shlwapi.h>
#pragma warning(pop)

#pragma comment(lib, "shlwapi.lib")
static SFillPropData fp_data;
#endif // XRSE_FACTORY_EXPORTS

#ifdef XRSE_FACTORY_EXPORTS
bool parse_bool(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read boolean value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TBOOLEAN, make_string("cannot read boolean value %s", identifier));
    return (luabind::object_cast<bool>(result));
}

BOOL is_combat_cover(shared_str const& table_id)
{
    if (table_id.size() == 0)
        return (FALSE);

    string256 temp;
    xr_strcpy(temp, "smart_covers.descriptions.");
    xr_strcat(temp, *table_id);

    luabind::object table, value;
    bool result = GEnv.ScriptEngine->function_object(temp, table, LUA_TTABLE);

    VERIFY2(result, make_string("bad or missing description in smart_cover [%s]", table_id.c_str()));
    if (luabind::type(table) != LUA_TTABLE)
    {
        VERIFY(luabind::type(table) != LUA_TNIL);
        return (TRUE);
    }

    value = table["is_combat_cover"];
    if (luabind::type(value) == LUA_TNIL)
    {
        Msg("! is_combat_cover flag not found for smart_cover [%s], forcing to \"true\"", table_id.c_str());
        return (TRUE);
    }

    return (parse_bool(table, "is_combat_cover") ? TRUE : FALSE);
}
#endif // XRSE_FACTORY_EXPORTS

CSE_SmartCover::CSE_SmartCover(LPCSTR section) : CSE_ALifeDynamicObject(section)
{
#ifdef XRSE_FACTORY_EXPORTS
    fp_data.inc();
#endif // XRSE_FACTORY_EXPORTS

    m_enter_min_enemy_distance = pSettings->r_float(section, "enter_min_enemy_distance");
    m_exit_min_enemy_distance = pSettings->r_float(section, "exit_min_enemy_distance");
    m_is_combat_cover = pSettings->r_bool(section, "is_combat_cover");
    m_can_fire = m_is_combat_cover ? true : pSettings->r_bool(section, "can_fire");
    m_need_to_reparse_loopholes = true;
}

CSE_SmartCover::~CSE_SmartCover()
{
#ifdef XRSE_FACTORY_EXPORTS
    fp_data.dec();
#endif // XRSE_FACTORY_EXPORTS
}

IServerEntityShape* CSE_SmartCover::shape() { return this; }
bool CSE_SmartCover::used_ai_locations() const /* noexcept */ { return true; }
bool CSE_SmartCover::can_save() const /* noexcept */ { return true; }
bool CSE_SmartCover::can_switch_online() const /* noexcept */ { return true; }
bool CSE_SmartCover::can_switch_offline() const /* noexcept */ { return false; }
bool CSE_SmartCover::interactive() const /* noexcept */ { return false; }
LPCSTR CSE_SmartCover::description() const { return (m_description.c_str()); }
#ifndef AI_COMPILER
void CSE_SmartCover::set_available_loopholes(luabind::object table) { m_available_loopholes = table; }
#endif // #ifndef AI_COMPILER

void CSE_SmartCover::STATE_Read(NET_Packet& tNetPacket, u16 size)
{
    inherited1::STATE_Read(tNetPacket, size);
    cform_read(tNetPacket);
    tNetPacket.r_stringZ(m_description);
    m_hold_position_time = tNetPacket.r_float();
    if (m_wVersion >= 120)
    {
        m_enter_min_enemy_distance = tNetPacket.r_float();
        m_exit_min_enemy_distance = tNetPacket.r_float();
    }

    if (m_wVersion >= 122)
        m_is_combat_cover = tNetPacket.r_u8();

    if (m_wVersion >= 128)
        m_can_fire = tNetPacket.r_u8();
}

void CSE_SmartCover::STATE_Write(NET_Packet& tNetPacket)
{
    inherited1::STATE_Write(tNetPacket);
    cform_write(tNetPacket);
    tNetPacket.w_stringZ(m_description);
    tNetPacket.w_float(m_hold_position_time);
    tNetPacket.w_float(m_enter_min_enemy_distance);
    tNetPacket.w_float(m_exit_min_enemy_distance);
    tNetPacket.w_u8((u8)m_is_combat_cover);
    tNetPacket.w_u8((u8)m_can_fire);
}

void CSE_SmartCover::UPDATE_Read(NET_Packet& tNetPacket) { inherited1::UPDATE_Read(tNetPacket); }
void CSE_SmartCover::UPDATE_Write(NET_Packet& tNetPacket) { inherited1::UPDATE_Write(tNetPacket); }
#ifndef XRGAME_EXPORTS
void CSE_SmartCover::FillProps(LPCSTR pref, PropItemVec& items)
{
#ifdef XRSE_FACTORY_EXPORTS
    PHelper().CreateFloat(items, PrepareKey(pref, *s_name, "hold position time"), &m_hold_position_time, 0.f, 60.f);
    RListValue* value = PHelper().CreateRList(items, PrepareKey(pref, *s_name, "description"), &m_description,
        &*fp_data.smart_covers.begin(), fp_data.smart_covers.size());
    value->OnChangeEvent.bind(this, &CSE_SmartCover::OnChangeDescription);

    PHelper().CreateFloat(
        items, PrepareKey(pref, *s_name, "enter min enemy distance"), &m_enter_min_enemy_distance, 0.f, 100.f);
    PHelper().CreateFloat(
        items, PrepareKey(pref, *s_name, "exit min enemy distance"), &m_exit_min_enemy_distance, 0.f, 100.f);

    if (is_combat_cover(m_description))
    {
        PHelper().CreateBOOL(items, PrepareKey(pref, *s_name, "is combat cover"), &m_is_combat_cover);
        PHelper().CreateBOOL(items, PrepareKey(pref, *s_name, "can fire"), &m_can_fire);
    }
#endif // #ifdef XRSE_FACTORY_EXPORTS
}
#endif // #ifndef XRGAME_EXPORTS

#ifdef XRSE_FACTORY_EXPORTS
void CSE_SmartCover::set_loopholes_table_checker(BOOLValue* value)
{
    value->OnChangeEvent.bind(this, &CSE_SmartCover::OnChangeLoopholes);
}

void CSE_SmartCover::OnChangeLoopholes(PropValue* sender)
{
    CScriptValueContainer::assign();
    m_need_to_reparse_loopholes = true;
}

void CSE_SmartCover::OnChangeDescription(PropValue* sender)
{
    set_editor_flag(flVisualChange);
    load_draw_data();
}

LPCSTR parse_string(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read string value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TSTRING, make_string("cannot read string value %s", identifier));
    return (luabind::object_cast<LPCSTR>(result));
}

Fvector parse_fvector(luabind::object const& table, LPCSTR identifier)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read vector value %s", identifier));
    return (luabind::object_cast<Fvector>(result));
}

float parse_float(luabind::object const& table, LPCSTR identifier, float const& min_threshold = flt_min,
    float const& max_threshold = flt_max)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    luabind::object lua_result = table[identifier];
    VERIFY2(luabind::type(lua_result) != LUA_TNIL, make_string("cannot read number value %s", identifier));
    VERIFY2(luabind::type(lua_result) == LUA_TNUMBER, make_string("cannot read number value %s", identifier));
    float result = luabind::object_cast<float>(lua_result);
    VERIFY2(result >= min_threshold, make_string("invalid read number value %s", identifier));
    VERIFY2(result <= max_threshold, make_string("invalid number value %s", identifier));
    return (result);
}

void parse_table(luabind::object const& table, LPCSTR identifier, luabind::object& result)
{
    VERIFY2(luabind::type(table) == LUA_TTABLE, "invalid loophole description passed");
    result = table[identifier];
    VERIFY2(luabind::type(result) != LUA_TNIL, make_string("cannot read table value %s", identifier));
    VERIFY2(luabind::type(result) == LUA_TTABLE, make_string("cannot read table value %s", identifier));
}

namespace smart_cover
{
static LPCSTR s_enter_loophole_id = "<__ENTER__>";
static LPCSTR s_exit_loophole_id = "<__EXIT__>";

shared_str transform_vertex(shared_str const& vertex_id, bool const& in)
{
    if (*vertex_id.c_str())
        return (vertex_id);

    if (in)
        return (s_enter_loophole_id);

    return (s_exit_loophole_id);
}

shared_str parse_vertex(luabind::object const& table, LPCSTR identifier, bool const& in)
{
    return (transform_vertex(parse_string(table, identifier), in));
}
} // namespace smart_cover

void CSE_SmartCover::set_enterable(shared_str const& id)
{
    class id_predicate
    {
        shared_str m_id;

    public:
        IC id_predicate(shared_str const& id) : m_id(id) {}
        IC bool operator()(SSCDrawHelper const& h) const { return (m_id._get() == h.string_identifier._get()); }
    };

    xr_vector<SSCDrawHelper>::iterator found = std::find_if(m_draw_data.begin(), m_draw_data.end(), id_predicate(id));
    // VERIFY2                       (found != m_draw_data.end(), id.c_str());
    if (found == m_draw_data.end())
        return;

    found->is_enterable = true;
}

void CSE_SmartCover::check_enterable_loopholes(shared_str const& description)
{
    string256 temp;
    xr_strcpy(temp, "smart_covers.descriptions.");
    xr_strcat(temp, m_description.c_str());
    xr_strcat(temp, ".transitions");

    luabind::object transitions;
    bool result = GEnv.ScriptEngine->function_object(temp, transitions, LUA_TTABLE);
    VERIFY2(result, make_string("bad or missing transitions table in smart_cover [%s]", temp));

    for (luabind::iterator I(transitions), E; I != E; ++I)
    {
        luabind::object table = *I;
        if (luabind::type(table) != LUA_TTABLE)
        {
            VERIFY(luabind::type(table) != LUA_TNIL);
            continue;
        }

        shared_str vertex_0 = smart_cover::parse_vertex(table, "vertex0", true);
        if (vertex_0 != smart_cover::transform_vertex("", true))
            continue;

        set_enterable(smart_cover::parse_vertex(table, "vertex1", false));
    }
}

class CSE_SmartVisual : public CSE_Visual
{
public:
    virtual CSE_Visual* __stdcall visual() { return (this); }
}; // class CSE_SmartVisual

void CSE_SmartCover::fill_visuals()
{
    delete_data(m_visuals);

    xr_vector<SSCDrawHelper>::iterator I = m_draw_data.begin();
    xr_vector<SSCDrawHelper>::iterator E = m_draw_data.end();
    for (; I != E; ++I)
    {
        if (!I->is_enterable)
            return;

        CSE_Visual* visual = new CSE_SmartVisual();
        visual->set_visual("actors" DELIMITER "stalker_neutral" DELIMITER "stalker_neutral_1");

        if (I->animation_id.size() == 0)
        {
            Msg("cover [%s] doesn't have idle_2_fire animation", I->string_identifier.c_str());
            return;
        }

        visual->startup_animation = I->animation_id;

        visual_data tmp;
        tmp.visual = visual;
        tmp.matrix.rotation(I->enter_direction, Fvector().set(0.f, 1.f, 0.f));
        tmp.matrix.c = I->point_position;

        m_visuals.push_back(tmp);
    }
}

void draw_frustum(CDUInterface* du, float FOV, float _FAR, float A, Fvector& P, Fvector& D, Fvector& U, u32 const& CL)
{
    float YFov = deg2rad(FOV * A);
    float XFov = deg2rad(FOV);

    // calc window extents in camera coords
    float wR = tanf(XFov * 0.5f);
    float wL = -wR;
    float wT = tanf(YFov * 0.5f);
    float wB = -wT;

    // calc x-axis (viewhoriz) and store cop
    // here we are assuring that vectors are perpendicular & normalized
    Fvector R, COP;
    D.normalize();
    R.crossproduct(D, U);
    R.normalize();
    U.crossproduct(R, D);
    U.normalize();
    COP.set(P);

    // calculate the corner vertices of the window
    Fvector sPts[4]; // silhouette points (corners of window)
    Fvector Offset, T;
    Offset.add(D, COP);

    sPts[0].mul(R, wR);
    T.mad(Offset, U, wT);
    sPts[0].add(T);
    sPts[1].mul(R, wL);
    T.mad(Offset, U, wT);
    sPts[1].add(T);
    sPts[2].mul(R, wL);
    T.mad(Offset, U, wB);
    sPts[2].add(T);
    sPts[3].mul(R, wR);
    T.mad(Offset, U, wB);
    sPts[3].add(T);

    // find projector direction vectors (from cop through silhouette pts)
    Fvector ProjDirs[4];
    ProjDirs[0].sub(sPts[0], COP);
    ProjDirs[1].sub(sPts[1], COP);
    ProjDirs[2].sub(sPts[2], COP);
    ProjDirs[3].sub(sPts[3], COP);

    Fvector _F[4];
    _F[0].mad(COP, ProjDirs[0], _FAR);
    _F[1].mad(COP, ProjDirs[1], _FAR);
    _F[2].mad(COP, ProjDirs[2], _FAR);
    _F[3].mad(COP, ProjDirs[3], _FAR);

    du->DrawLine(COP, _F[0], CL);
    du->DrawLine(COP, _F[1], CL);
    du->DrawLine(COP, _F[2], CL);
    du->DrawLine(COP, _F[3], CL);

    du->DrawLine(_F[0], _F[1], CL);
    du->DrawLine(_F[1], _F[2], CL);
    du->DrawLine(_F[2], _F[3], CL);
    du->DrawLine(_F[3], _F[0], CL);
}

shared_str animation_id(luabind::object table)
{
    for (luabind::iterator i(table), e; i != e; ++i)
    {
        luabind::object string = *i;
        if (luabind::type(string) != LUA_TSTRING)
        {
            VERIFY(luabind::type(string) != LUA_TNIL);
            continue;
        }

        return (luabind::object_cast<LPCSTR>(string));
    }

    return ("");
}

void CSE_SmartCover::load_draw_data()
{
    string256 temp;
    xr_strcpy(temp, "smart_covers.descriptions.");
    xr_strcat(temp, m_description.c_str());
    xr_strcat(temp, ".loopholes");

    m_draw_data.clear();

    luabind::object loopholes;
    bool result = GEnv.ScriptEngine->function_object(temp, loopholes, LUA_TTABLE);

    if (!result)
    {
        Msg("no or invalid smart cover description (bad or missing loopholes table in smart_cover [%s])", temp);
        return;
        //      VERIFY2                 (result, make_string("bad or missing loopholes table in smart_cover [%s]",
        //      temp));
    }

    for (luabind::iterator I(loopholes), E; I != E; ++I)
    {
        bool loophole_exist = true;
        if (m_available_loopholes.is_valid())
        {
            for (luabind::iterator i(m_available_loopholes), e; i != e; ++i)
            {
                LPCSTR const loophole_id = luabind::object_cast<LPCSTR>(i.key());
                shared_str descr_loophole_id = parse_string(*I, "id");
                if (xr_strcmp(loophole_id, descr_loophole_id))
                    continue;
                if (!luabind::object_cast<bool>(*i))
                    loophole_exist = false;
                break;
            }
        }
        if (!loophole_exist)
            continue;

        luabind::object table = *I;
        if (luabind::type(table) != LUA_TTABLE)
        {
            VERIFY(luabind::type(table) != LUA_TNIL);
            continue;
        }
        m_draw_data.resize(m_draw_data.size() + 1);
        SSCDrawHelper& H = m_draw_data.back();

        H.string_identifier = parse_string(table, "id");
        H.point_position = parse_fvector(table, "fov_position");
        H.is_enterable = false;
        H.fov_direction = parse_fvector(table, "fov_direction");

        if (H.fov_direction.square_magnitude() < EPS_L)
        {
            Msg("! fov direction for loophole %s is setup incorrectly", H.string_identifier.c_str());
            H.fov_direction.set(0.f, 0.f, 1.f);
        }
        else
            H.fov_direction.normalize();

        H.enter_direction = parse_fvector(table, "enter_direction");

        if (H.enter_direction.square_magnitude() < EPS_L)
        {
            Msg("! enter direction for loophole %s is setup incorrectly", H.string_identifier.c_str());
            H.enter_direction.set(0.f, 0.f, 1.f);
        }
        else
            H.enter_direction.normalize();

        H.fov = parse_float(table, "fov", 0.f, 360.f);
        H.range = parse_float(table, "range", 0.f);

        /*      luabind::object transitions;
                parse_table     (table, "transitions", transitions);
                luabind::object::iterator I = transitions.begin();
                luabind::object::iterator E = transitions.end();
                for ( ; I != E; ++I) {
                    luabind::object transition = *I;
                    VERIFY2             (transition.type() == LUA_TTABLE, "invalid loophole description passed");
                    shared_str          action_from = smart_cover::parse_vertex(transition, "action_from", true);
                    if (action_from != "idle")
                        continue;
                    shared_str          action_to = smart_cover::parse_vertex(transition, "action_to", false);
                    if (action_to != "fire")
                        continue;

                    luabind::object     result;
                    parse_table         (transition, "animations", result);

                    H.animation_id      = animation_id(result);
                    break;
                }
        */

        H.animation_id = make_string("loophole_%s_visual", H.string_identifier.c_str()).c_str();
    }

    check_enterable_loopholes(m_description);
    fill_visuals();
}

void CSE_SmartCover::on_render(
    CDUInterface* du, IServerEntityLEOwner* owner, bool bSelected, const Fmatrix& parent, int priority, bool strictB2F)
{
    inherited1::on_render(du, owner, bSelected, parent, priority, strictB2F);
    if (!((1 == priority) && (false == strictB2F)))
        return;

    if (m_need_to_reparse_loopholes && m_description.size())
    {
        OnChangeDescription(nullptr);
        m_need_to_reparse_loopholes = false;
    }

    if (!bSelected)
        return;

    xr_vector<SSCDrawHelper>::iterator it = m_draw_data.begin();
    xr_vector<SSCDrawHelper>::iterator it_e = m_draw_data.end();

    for (; it != it_e; ++it)
    {
        SSCDrawHelper& H = *it;
        Fvector pos = H.point_position;
        parent.transform_tiny(pos);

        du->OutText(pos, H.string_identifier.c_str(), color_rgba(255, 255, 255, 255));

        // du->DrawBox(H.point_position,Fvector().set(0.2f,0.2f,0.2f),TRUE,TRUE,color_rgba(255,0,0,80),color_rgba(0,255,0,255));
        // du->DrawFlag(H.point_position, 0, 1.0f, 1, 1, color_rgba(0,255,0,255), FALSE);
        // du->DrawCylinder(Fidentity, pos, Fvector().set(0.f, 1.f, 0.f), 1.f, .05f, color_rgba(0,255,0,255),
        // color_rgba(0,255,0,255), TRUE, FALSE);

        Fvector dir = H.fov_direction;
        parent.transform_dir(dir);
        Fvector up = parent.j;
        draw_frustum(du, H.fov, H.range, 1.f, pos, dir, up, color_rgba(255, 0, 0, 255));
    }
}
#endif // #ifdef XRSE_FACTORY_EXPORTS
