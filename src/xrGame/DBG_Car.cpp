#include "StdAfx.h"
#ifdef DEBUG
#include "PHDebug.h"
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Car.h"
#include "Level.h"
#include "xrUICore/ui_base.h"
#include "xrEngine/GameFont.h"

void CCar::InitDebug()
{
    m_dbg_power_rpm.Clear();
    m_dbg_torque_rpm.Clear();
    m_dbg_dynamic_plot = 0;
    b_plots = false;
}
void CCar::DbgSheduleUpdate()
{
    if (ph_dbg_draw_mask.test(phDbgDrawCarPlots) && m_pPhysicsShell && OwnerActor() &&
        static_cast<IGameObject*>(Owner()) == Level().CurrentViewEntity())
    {
        DbgCreatePlots();
    }
    else
    {
        DBgClearPlots();
    }
}

static float torq_pow_max_ratio = 1.f;
static float rpm_pow_max_ratio = 1.f;

void CCar::DbgCreatePlots()
{
    if (b_plots)
        return;
    eStateDrive state = e_state_drive;
    e_state_drive = drive;
    //////////////////////////////
    int y_pos = 0, y_w = 100;
    m_dbg_power_rpm.Init(CFunctionGraph::type_function(this, &CCar::Parabola), m_min_rpm, m_max_rpm, 0, y_pos, 500, y_w,
        1000, color_xrgb(0, 0, 255));
    m_dbg_power_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(255, 0, 0));
    m_dbg_power_rpm.AddMarker(CStatGraph::stHor, 0, color_xrgb(0, 0, 255));
    m_dbg_power_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(0, 0, 0));

    m_dbg_power_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(127, 0, 0));
    m_dbg_power_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(0, 0, 127));

    y_pos += y_w + 10;

    m_dbg_torque_rpm.Init(
        CFunctionGraph::type_function(this, &CCar::TorqueRpmFun), m_min_rpm, m_max_rpm, 0, y_pos, 500, y_w, 1000);
    m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(255, 0, 0));
    m_dbg_torque_rpm.AddMarker(CStatGraph::stHor, 0, color_xrgb(0, 255, 0));
    m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(0, 0, 0));

    m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(127, 0, 0));
    m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, 0, color_xrgb(0, 0, 127));

    y_pos += y_w + 10;

    if (b_auto_switch_transmission && ph_dbg_draw_mask.test(phDbgDrawCarAllTrnsm))
    {
        xr_vector<Fvector>::iterator i = m_gear_ratious.begin() + 1, e = m_gear_ratious.end();
        for (; i < e; i++)
        {
            float r = 4 * m_dbg_torque_rpm.ResolutionX();
            m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, (*i)[1] + r, color_xrgb(255, 255, 0));
            m_dbg_torque_rpm.AddMarker(CStatGraph::stVert, (*i)[2] + r, color_xrgb(0, 255, 255));
            r = 4 * m_dbg_power_rpm.ResolutionX();
            m_dbg_power_rpm.AddMarker(CStatGraph::stVert, (*i)[1] + r, color_xrgb(255, 255, 0));
            m_dbg_power_rpm.AddMarker(CStatGraph::stVert, (*i)[2] + r, color_xrgb(0, 255, 255));
        }
    }
    //--------------------------------------
    m_dbg_dynamic_plot = new CStatGraph();
    m_dbg_dynamic_plot->SetRect(0, y_pos, 500, y_w, color_xrgb(255, 255, 255), color_xrgb(255, 255, 255));
    m_dbg_dynamic_plot->SetMinMax(Parabola(m_min_rpm), m_max_power, 1000);
    m_dbg_dynamic_plot->AppendSubGraph(CStatGraph::stCurve);
    torq_pow_max_ratio = Parabola(m_torque_rpm) / m_torque_rpm / m_max_power;

    m_dbg_dynamic_plot->AppendSubGraph(CStatGraph::stCurve);
    rpm_pow_max_ratio = m_max_rpm / m_max_power;
    //--------------------------------------
    m_dbg_dynamic_plot->AddMarker(CStatGraph::stHor, 0, color_xrgb(255, 0, 0));
    xr_vector<Fvector>::iterator i = m_gear_ratious.begin() + 1, e = m_gear_ratious.end();
    for (; i < e; i++)
    {
        m_dbg_dynamic_plot->AddMarker(CStatGraph::stHor, (*i)[1] / rpm_pow_max_ratio, color_xrgb(127, 0, 0));
        m_dbg_dynamic_plot->AddMarker(CStatGraph::stHor, (*i)[2] / rpm_pow_max_ratio, color_xrgb(0, 0, 127));
    }
    //////////////////////////////
    e_state_drive = state;
    b_plots = true;
}
void CCar::DBgClearPlots()
{
    if (!b_plots)
        return;
    ////////////////////////////////
    m_dbg_power_rpm.Clear();
    m_dbg_torque_rpm.Clear();
    xr_delete(m_dbg_dynamic_plot);
    ////////////////////////////////
    b_plots = false;
}

void CCar::DbgUbdateCl()
{
    if (m_pPhysicsShell && OwnerActor() && static_cast<IGameObject*>(Owner()) == Level().CurrentViewEntity())
    {
        if (ph_dbg_draw_mask.test(phDbgDrawCarDynamics))
        {
            Fvector v;
            m_pPhysicsShell->get_LinearVel(v);
            string32 s;
            xr_sprintf(s, "speed, %f km/hour", v.magnitude() / 1000.f * 3600.f);
            UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            UI().Font().pFontStat->OutSet(120, 530);
            UI().Font().pFontStat->OutNext(s);
            UI().Font().pFontStat->SetColor(
                color_xrgb(255, !b_transmission_switching * 255, !b_transmission_switching * 255));
            UI().Font().pFontStat->OutNext("Transmission num:      [%d]", m_current_transmission_num);
            UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            UI().Font().pFontStat->OutNext("gear ratio:			  [%3.2f]", m_current_gear_ratio);
            UI().Font().pFontStat->OutNext("Power:      [%3.2f]", m_current_engine_power / (0.8f * 1000.f));
            UI().Font().pFontStat->OutNext("rpm:      [%3.2f]", m_current_rpm / (1.f / 60.f * 2.f * M_PI));
            UI().Font().pFontStat->OutNext("wheel torque:      [%3.2f]", RefWheelCurTorque());
            UI().Font().pFontStat->OutNext("engine torque:      [%3.2f]", EngineCurTorque());
            UI().Font().pFontStat->OutNext("fuel:      [%3.2f]", m_fuel);
            if (b_clutch)
            {
                UI().Font().pFontStat->SetColor(color_xrgb(0, 255, 0));
                UI().Font().pFontStat->OutNext("CLUTCH");
                UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            }
            if (b_engine_on)
            {
                UI().Font().pFontStat->SetColor(color_xrgb(0, 255, 0));
                UI().Font().pFontStat->OutNext("ENGINE ON");
                UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            }
            if (b_stalling)
            {
                UI().Font().pFontStat->SetColor(color_xrgb(255, 0, 0));
                UI().Font().pFontStat->OutNext("STALLING");
                UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            }
            if (b_starting)
            {
                UI().Font().pFontStat->SetColor(color_xrgb(255, 0, 0));
                UI().Font().pFontStat->OutNext("STARTER");
                UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            }
            if (b_breaks)
            {
                UI().Font().pFontStat->SetColor(color_xrgb(255, 0, 0));
                UI().Font().pFontStat->OutNext("BREAKS");
                UI().Font().pFontStat->SetColor(color_rgba(0xff, 0xff, 0xff, 0xff));
            }
            // HUD().pFontStat->OutNext("Vel Magnitude: [%3.2f]",m_PhysicMovementControl->GetVelocityMagnitude());
            // HUD().pFontStat->OutNext("Vel Actual:    [%3.2f]",m_PhysicMovementControl->GetVelocityActual());
        }

        if (ph_dbg_draw_mask.test(phDbgDrawCarPlots) && b_plots)
        {
            float cur_torque = EngineCurTorque();
            m_dbg_dynamic_plot->AppendItem(m_current_engine_power, color_xrgb(0, 0, 255));
            m_dbg_dynamic_plot->AppendItem(cur_torque / torq_pow_max_ratio, color_xrgb(0, 255, 0), 1);
            m_dbg_dynamic_plot->AppendItem(m_current_rpm / rpm_pow_max_ratio, color_xrgb(255, 0, 0), 2);

            m_dbg_dynamic_plot->UpdateMarkerPos(0, m_current_rpm / rpm_pow_max_ratio);

            float engine_wheels_rpm = EngineRpmFromWheels();
            m_dbg_power_rpm.UpdateMarker(0, m_current_rpm);
            m_dbg_power_rpm.UpdateMarker(1, m_current_engine_power);
            m_dbg_power_rpm.UpdateMarker(2, engine_wheels_rpm);
            m_dbg_power_rpm.UpdateMarker(3, m_gear_ratious[m_current_transmission_num][2]);
            m_dbg_power_rpm.UpdateMarker(4, m_gear_ratious[m_current_transmission_num][1]);

            m_dbg_torque_rpm.UpdateMarker(0, m_current_rpm);
            m_dbg_torque_rpm.UpdateMarker(1, cur_torque);
            m_dbg_torque_rpm.UpdateMarker(2, engine_wheels_rpm);
            m_dbg_torque_rpm.UpdateMarker(3, m_gear_ratious[m_current_transmission_num][2]);
            m_dbg_torque_rpm.UpdateMarker(4, m_gear_ratious[m_current_transmission_num][1]);
        }
    }
}

#endif
