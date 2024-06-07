#include "stdafx.h"
#include "IGame_Level.h"
#include "IGame_Persistent.h"

#include "GameFont.h"
#include "FDemoRecord.h"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "Render.h"
#include "CustomHUD.h"
#include "CameraManager.h"

constexpr cpcstr DEMO_RECORD_HELP_FONT = "ui_font_letterica18_russian"; // "ui_font_graffiti19_russian";

ENGINE_API extern bool g_bDisableRedText;
static Flags32 s_hud_flag = {};
static Flags32 s_dev_flags = {};
static u32     s_window_mode = {};

bool stored_weapon;
bool stored_cross;
bool stored_red_text;

CDemoRecord* xrDemoRecord = 0;
CDemoRecord::force_position CDemoRecord::g_position = {false, {0, 0, 0}};

Fbox curr_lm_fbox;
void setup_lm_screenshot_matrices()
{
    psHUD_Flags.assign(0);

    // build camera matrix
    Fbox bb = curr_lm_fbox;
    bb.getcenter(Device.vCameraPosition);

    Device.vCameraDirection.set(0.f, -1.f, 0.f);
    Device.vCameraTop.set(0.f, 0.f, 1.f);
    Device.vCameraRight.set(1.f, 0.f, 0.f);
    Device.mView.build_camera_dir(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);

    bb.xform(Device.mView);
    // build project matrix
    Device.mProject.build_projection_ortho(bb.vMax.x - bb.vMin.x, bb.vMax.y - bb.vMin.y, bb.vMin.z, bb.vMax.z);
}

Fbox get_level_screenshot_bound()
{
    Fbox res = g_pGameLevel->ObjectSpace.GetBoundingVolume();
    if (g_pGameLevel->pLevel->section_exist("level_map"))
    {
        Fvector4 res2d = g_pGameLevel->pLevel->r_fvector4("level_map", "bound_rect");
        res.vMin.x = res2d.x;
        res.vMin.z = res2d.y;

        res.vMax.x = res2d.z;
        res.vMax.z = res2d.w;
    }

    return res;
}

pcstr GetFontTexName(pcstr section)
{
    static const char* tex_names[] = { "texture800", "texture", "texture1600" };
    int def_idx = 1; // default 1024x768
    int idx = def_idx;

#if 0
    u32 w = Device.dwWidth;

    if (w <= 800)
        idx = 0;
    else if (w <= 1280)
        idx = 1;
    else
        idx = 2;
#else
    u32 h = Device.dwHeight;

    if (h <= 600)
        idx = 0;
    else if (h <= 1024)
        idx = 1;
    else
        idx = 2;
#endif

    while (idx >= 0)
    {
        if (pSettings->line_exist(section, tex_names[idx]))
            return pSettings->r_string(section, tex_names[idx]);
        --idx;
    }
    return pSettings->r_string(section, tex_names[def_idx]);
}

CDemoRecord::CDemoRecord(const char* name, float life_time)
    : CEffectorCam(cefDemo, life_time /*,false*/),
      m_speed(speed_0),
      m_angle_speed(speed_0),
      m_Font(pSettings->r_string(DEMO_RECORD_HELP_FONT, "shader"), GetFontTexName(DEMO_RECORD_HELP_FONT))
{
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);

    stored_red_text = g_bDisableRedText;
    g_bDisableRedText = true;
    m_iLMScreenshotFragment = -1;
    /*
     stored_weapon = psHUD_Flags.test(HUD_WEAPON);
     stored_cross = psHUD_Flags.test(HUD_CROSSHAIR);
     psHUD_Flags.set(HUD_WEAPON, false);
     psHUD_Flags.set(HUD_CROSSHAIR, false);
     */
    m_b_redirect_input_to_level = false;
    xr_unlink(name);
    file = FS.w_open(name);
    if (file)
    {
        g_position.set_position = false;
        IR_Capture(); // capture input
        m_Camera.invert(Device.mView);

        // parse yaw
        Fvector& dir = m_Camera.k;
        Fvector DYaw;
        DYaw.set(dir.x, 0.f, dir.z);
        DYaw.normalize_safe();
        if (DYaw.x < 0)
            m_HPB.x = acosf(DYaw.z);
        else
            m_HPB.x = 2 * PI - acosf(DYaw.z);

        // parse pitch
        dir.normalize_safe();
        m_HPB.y = asinf(dir.y);
        m_HPB.z = 0;

        m_Position.set(m_Camera.c);

        m_vVelocity.set(0, 0, 0);
        m_vAngularVelocity.set(0, 0, 0);
        iCount = 0;

        m_vT.set(0, 0, 0);
        m_vR.set(0, 0, 0);
        m_bMakeCubeMap = false;
        m_bMakeScreenshot = false;
        m_bMakeLevelMap = false;

        m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
        m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
        m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
        m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
        m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
        m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
        m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
        m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");
    }
    else
    {
        fLifeTime = -1;
    }
}

CDemoRecord::~CDemoRecord()
{
    if (file)
    {
        IR_Release(); // release input
        FS.w_close(file);
    }
    g_bDisableRedText = stored_red_text;

    Device.seqRender.Remove(this);
}

// +X, -X, +Y, -Y, +Z, -Z
static Fvector cmNorm[6] = {
    {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};
static Fvector cmDir[6] = {
    {1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, -1.f}};

void CDemoRecord::MakeScreenshotFace()
{
    switch (m_Stage)
    {
    case 0:
        s_hud_flag.assign(psHUD_Flags);
        psHUD_Flags.assign(0);
        break;
    case 1:
        GEnv.Render->Screenshot();
        psHUD_Flags.assign(s_hud_flag);
        m_bMakeScreenshot = false;
        break;
    }
    m_Stage++;
}

void GetLM_BBox(Fbox& bb, int Step)
{
    float half_x = bb.vMin.x + (bb.vMax.x - bb.vMin.x) / 2;
    float half_z = bb.vMin.z + (bb.vMax.z - bb.vMin.z) / 2;
    switch (Step)
    {
    case 0:
    {
        bb.vMax.x = half_x;
        bb.vMin.z = half_z;
    }
    break;
    case 1:
    {
        bb.vMin.x = half_x;
        bb.vMin.z = half_z;
    }
    break;
    case 2:
    {
        bb.vMax.x = half_x;
        bb.vMax.z = half_z;
    }
    break;
    case 3:
    {
        bb.vMin.x = half_x;
        bb.vMax.z = half_z;
    }
    break;
    default: {
    }
    break;
    }
};

void CDemoRecord::MakeLevelMapProcess()
{
    switch (m_Stage)
    {
    case 0:
    {
        s_window_mode = psDeviceMode.WindowStyle;
        s_dev_flags = psDeviceFlags;
        s_hud_flag.assign(psHUD_Flags);
        psDeviceFlags.zero();
        psDeviceFlags.set(rsClearBB | rsDrawStatic, true);
        psDeviceMode.WindowStyle = rsFullscreen;
        if (psDeviceMode.WindowStyle != s_window_mode)
            Device.Reset();
    }
    break;

    case DEVICE_RESET_PRECACHE_FRAME_COUNT + 30:
    {
        setup_lm_screenshot_matrices();

        string_path tmp;
        if (m_iLMScreenshotFragment == -1)
            xr_sprintf(tmp, sizeof(tmp), "map_%s", *g_pGameLevel->name());
        else
            xr_sprintf(tmp, sizeof(tmp), "map_%s#%d", *g_pGameLevel->name(), m_iLMScreenshotFragment);

        if (m_iLMScreenshotFragment != -1)
        {
            ++m_iLMScreenshotFragment;

            if (m_iLMScreenshotFragment != 4)
            {
                curr_lm_fbox = get_level_screenshot_bound();
                GetLM_BBox(curr_lm_fbox, m_iLMScreenshotFragment);
                m_Stage -= 20;
            }
        }

        GEnv.Render->Screenshot(IRender::SM_FOR_LEVELMAP, tmp);

        if (m_iLMScreenshotFragment == -1 || m_iLMScreenshotFragment == 4)
        {
            psHUD_Flags.assign(s_hud_flag);
            psDeviceFlags = s_dev_flags;

            const bool bDevReset = psDeviceMode.WindowStyle != s_window_mode;
            psDeviceMode.WindowStyle = s_window_mode;
            if (bDevReset)
                Device.Reset();

            if (!m_CurrentWeatherCycle.empty())
            {
                g_pGamePersistent->Environment().SetWeather(m_CurrentWeatherCycle, true);
                m_CurrentWeatherCycle = nullptr;
            }

            m_bMakeLevelMap = false;
            m_iLMScreenshotFragment = -1;
        }
    }
    break;
    default: { setup_lm_screenshot_matrices();
    }
    break;
    }
    m_Stage++;
}

void CDemoRecord::MakeCubeMapFace(Fvector& D, Fvector& N)
{
    string32 buf;
    switch (m_Stage)
    {
    case 0:
        N.set(cmNorm[m_Stage]);
        D.set(cmDir[m_Stage]);
        s_hud_flag.assign(psHUD_Flags);
        psHUD_Flags.assign(0);
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        N.set(cmNorm[m_Stage]);
        D.set(cmDir[m_Stage]);
        GEnv.Render->Screenshot(IRender::SM_FOR_CUBEMAP, xr_itoa(m_Stage, buf, 10));
        break;
    case 6:
        GEnv.Render->Screenshot(IRender::SM_FOR_CUBEMAP, xr_itoa(m_Stage, buf, 10));
        N.set(m_Camera.j);
        D.set(m_Camera.k);
        psHUD_Flags.assign(s_hud_flag);
        m_bMakeCubeMap = false;
        break;
    }
    m_Stage++;
}

bool CDemoRecord::ProcessCam(SCamEffectorInfo& info)
{
    info.dont_apply = false;
    if (0 == file)
        return true;

    if (m_bMakeScreenshot)
    {
        MakeScreenshotFace();
        // update camera
        info.n.set(m_Camera.j);
        info.d.set(m_Camera.k);
        info.p.set(m_Camera.c);
    }
    else if (m_bMakeLevelMap)
    {
        MakeLevelMapProcess();
        info.dont_apply = true;
    }
    else if (m_bMakeCubeMap)
    {
        MakeCubeMapFace(info.d, info.n);
        info.p.set(m_Camera.c);
        info.fAspect = 1.f;
    }
    else
    {
        if (IR_GetKeyState(SDL_SCANCODE_F1))
        {
            m_Font.SetColor(color_rgba(255, 0, 0, 255));
            m_Font.SetAligment(CGameFont::alCenter);
            m_Font.OutSetI(0, -.05f);
            m_Font.OutNext("%s", "RECORDING");
            m_Font.OutNext("Key frames count: %d", iCount);
            m_Font.SetAligment(CGameFont::alLeft);
            m_Font.OutSetI(-0.2f, +.05f);
            m_Font.OutNext("SPACE");
            m_Font.OutNext("BACK");
            m_Font.OutNext("ESC");
            m_Font.OutNext("F11");
            m_Font.OutNext("LCONTROL+F11");
            m_Font.OutNext("F12");
            m_Font.SetAligment(CGameFont::alLeft);
            m_Font.OutSetI(0, +.05f);
            m_Font.OutNext("= Append Key");
            m_Font.OutNext("= Cube Map");
            m_Font.OutNext("= Quit");
            m_Font.OutNext("= Level Map ScreenShot");
            m_Font.OutNext("= Level Map ScreenShot(High Quality)");
            m_Font.OutNext("= ScreenShot");
        }

        m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
        m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

        float speed = m_fSpeed1, ang_speed = m_fAngSpeed1;
        switch (m_speed)
        {
        case speed_0: speed = m_fSpeed0; break;
        case speed_1: speed = m_fSpeed1; break;
        case speed_2: speed = m_fSpeed2; break;
        case speed_3: speed = m_fSpeed3; break;
        default: NODEFAULT;
        }
        switch (m_angle_speed)
        {
        case speed_0: ang_speed = m_fAngSpeed0; break;
        case speed_1: ang_speed = m_fAngSpeed1; break;
        case speed_2: ang_speed = m_fAngSpeed2; break;
        case speed_3: ang_speed = m_fAngSpeed3; break;
        default: NODEFAULT;
        }

        m_vT.mul(m_vVelocity, Device.fTimeDelta * speed);
        m_vR.mul(m_vAngularVelocity, Device.fTimeDelta * ang_speed);

        m_HPB.x -= m_vR.y;
        m_HPB.y -= m_vR.x;
        m_HPB.z += m_vR.z;
        if (g_position.set_position)
        {
            m_Position.set(g_position.p);
            g_position.set_position = false;
        }
        else
            g_position.p.set(m_Position);
        // move
        Fvector vmove;

        vmove.set(m_Camera.k);
        vmove.normalize_safe();
        vmove.mul(m_vT.z);
        m_Position.add(vmove);

        vmove.set(m_Camera.i);
        vmove.normalize_safe();
        vmove.mul(m_vT.x);
        m_Position.add(vmove);

        vmove.set(m_Camera.j);
        vmove.normalize_safe();
        vmove.mul(m_vT.y);
        m_Position.add(vmove);

        m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);
        m_Camera.translate_over(m_Position);

        // update camera
        info.n.set(m_Camera.j);
        info.d.set(m_Camera.k);
        info.p.set(m_Camera.c);

        fLifeTime -= Device.fTimeDelta;

        m_vT.set(0, 0, 0);
        m_vR.set(0, 0, 0);
    }
    return true;
}

void CDemoRecord::IR_OnKeyboardPress(int dik)
{
    if (dik == SDL_SCANCODE_PERIOD)
        m_b_redirect_input_to_level = !m_b_redirect_input_to_level;

    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnKeyboardPress(dik);
        return;
    }

    if (dik == SDL_SCANCODE_BACKSPACE)
        MakeCubemap();
    else if (dik == SDL_SCANCODE_F11)
        MakeLevelMapScreenshot(IR_GetKeyState(SDL_SCANCODE_LCTRL));

    switch (GetBindedAction(dik))
    {
    case kACCEL:
        m_speed = speed_0;
        m_angle_speed = speed_0;
        break;

    case kSPRINT_TOGGLE:
        m_speed = speed_2;
        m_angle_speed = speed_2;
        break;

    case kCROUCH:
    case kCROUCH_TOGGLE:
        m_speed = speed_3;
        m_angle_speed = speed_3;
        break;

    default:
    {
        switch (dik)
        {
        case SDL_SCANCODE_LSHIFT:
            m_speed = speed_0;
            m_angle_speed = speed_0;
            break;

        case SDL_SCANCODE_LALT:
            m_speed = speed_2;
            m_angle_speed = speed_2;
            break;

        case SDL_SCANCODE_LCTRL:
            m_speed = speed_3;
            m_angle_speed = speed_3;
            break;
        } // switch (dik)
        break;
    }
    } // switch (GetBindedAction(dik))

    switch (GetBindedAction(dik))
    {
    case kCONSOLE:
        Console->Show();
        break;

    case kJUMP:
        RecordKey();
        break;

    case kSCREENSHOT:
        MakeScreenshot();
        break;

    case kQUIT:
        fLifeTime = -1;
        break;

#ifndef MASTER_GOLD
    case kENTER:
    {
        IGameObject* entity = g_pGameLevel->CurrentEntity();
        if (entity)
        {
            entity->ForceTransformAndDirection(m_Camera);
            fLifeTime = -1;
        }
        break;
    }
#endif

    case kPAUSE:
        Device.Pause(!Device.Paused(), true, true, "demo_record");
        break;
    }
}

static void update_whith_timescale(Fvector& v, const Fvector& v_delta)
{
    VERIFY(!fis_zero(Device.time_factor()));
    float scale = 1.f / Device.time_factor();
    v.mad(v, v_delta, scale);
}

void CDemoRecord::IR_OnKeyboardHold(int dik)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnKeyboardHold(dik);
        return;
    }
    Fvector vT_delta = Fvector().set(0, 0, 0);
    Fvector vR_delta = Fvector().set(0, 0, 0);

    switch (dik)
    {
    case SDL_SCANCODE_KP_1:
        vT_delta.x -= 1.0f;
        break; // Slide Left

    case SDL_SCANCODE_KP_3:
        vT_delta.x += 1.0f;
        break; // Slide Right

    // rotation
    case SDL_SCANCODE_KP_2:
        vR_delta.x -= 1.0f;
        break; // Pitch Down

    case SDL_SCANCODE_KP_8:
        vR_delta.x += 1.0f;
        break; // Pitch Up

    case SDL_SCANCODE_KP_6:
        vR_delta.y += 1.0f;
        break; // Turn Left

    case SDL_SCANCODE_KP_4:
        vR_delta.y -= 1.0f;
        break; // Turn Right

    case SDL_SCANCODE_KP_9:
        vR_delta.z -= 2.0f;
        break; // Turn Right

    case SDL_SCANCODE_KP_7:
        vR_delta.z += 2.0f;
        break; // Turn Right

    default:
    {
        switch (GetBindedAction(dik))
        {
        case kWPN_FIRE:
            vT_delta.z += 1.0f;
            break; // Move Backward

        case kWPN_ZOOM:
            vT_delta.z -= 1.0f;
            break; // Move Forward

        case kL_STRAFE:
        case kLEFT:
            vT_delta.x -= 1.0f;
            break; // Slide Left

        case kR_STRAFE:
        case kRIGHT:
            vT_delta.x += 1.0f;
            break; // Slide Right

        case kBACK:
        case kDOWN:
            vT_delta.y -= 1.0f;
            break; // Slide Down

        case kFWD:
        case kUP:
            vT_delta.y += 1.0f;
            break; // Slide Up

        // rotation
        case kL_LOOKOUT:
            vR_delta.y += 1.0f;
            break; // Turn Left

        case kR_LOOKOUT:
            vR_delta.y -= 1.0f;
            break; // Turn Right
        } // switch (GetBindedAction(dik))
        break;
    }
    } // switch (dik)

    update_whith_timescale(m_vT, vT_delta);
    update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnKeyboardRelease(int dik)
{
    switch (GetBindedAction(dik))
    {
    case kACCEL:
    case kSPRINT_TOGGLE:
    case kCROUCH:
    case kCROUCH_TOGGLE:
        goto set_normal_speed;

    default:
    {
        switch (dik)
        {
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_LALT:
        case SDL_SCANCODE_LCTRL:
        set_normal_speed:
            m_speed = speed_1;
            m_angle_speed = speed_1;
            break;
        } // switch (dik)
        break;
    }
    } // switch (GetBindedAction(dik))
}

void CDemoRecord::OnAxisMove(float x, float y, float scale, bool invert)
{
    Fvector vR_delta = Fvector().set(0, 0, 0);
    if (!fis_zero(x) || !fis_zero(y))
    {
        vR_delta.y += x * scale; // heading
        vR_delta.x += (invert ? -1.f : 1.f) * y * scale * (3.f / 4.f); // pitch
    }
    update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnMouseMove(dx, dy);
        return;
    }

    const float scale = .5f; // psMouseSens;
    OnAxisMove(float(dx), float(dy), scale, psMouseInvert.test(1));
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnMouseHold(btn);
        return;
    }
    IR_OnKeyboardHold(btn);
}

void CDemoRecord::IR_OnControllerPress(int key, float x, float y)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnControllerPress(key, x, y);
        return;
    }

    IR_OnKeyboardPress(key);
}

void CDemoRecord::IR_OnControllerHold(int key, float x, float y)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnControllerHold(key, x, y);
        return;
    }

    const float look = std::max(std::abs(x), std::abs(y));
    movement_speed speed = speed_1;
    if (look >= 90.f)
        speed = speed_3;
    else if (look >= 75.f)
        speed = speed_2;
    else if (look < 45.f)
        speed = speed_0;

    switch (GetBindedAction(key))
    {
    case kLOOK_AROUND:
    {
        m_angle_speed = speed;
        const float scale = .05f; // psControllerStickSens;
        OnAxisMove(x, y, scale, psControllerInvertY.test(1));
        break;
    }

    case kWPN_FIRE:
    {
        m_speed = speed;
        Fvector vT_delta = Fvector().set(0, 0, 1.0f);
        update_whith_timescale(m_vT, vT_delta);
        break; // Move Backward
    }

    case kWPN_ZOOM:
    {
        m_speed = speed;
        Fvector vT_delta = Fvector().set(0, 0, -1.0f);
        update_whith_timescale(m_vT, vT_delta);
        break; // Move Forward
    }

    case kMOVE_AROUND:
    {
        m_speed = speed;
        Fvector vT_delta = Fvector().set(0, 0, 0);

        if (!fis_zero(x))
        {
            if (x > 35.f)
                vT_delta.x += 1.0f;
            else if (x < -35.f)
                vT_delta.x -= 1.0f;
        }
        if (!fis_zero(y))
        {
            if (y > 35.f)
                vT_delta.y -= 1.0f;
            else if (y < -35.f)
                vT_delta.y += 1.0f;
        }

        update_whith_timescale(m_vT, vT_delta);
        break;
    }

    default:
        IR_OnKeyboardHold(key);
        break;
    }
}

void CDemoRecord::IR_OnControllerRelease(int key, float x, float y)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnControllerRelease(key, x, y);
        return;
    }

    switch (GetBindedAction(key))
    {
    case kLOOK_AROUND:
        m_angle_speed = speed_1;
        break;

    case kMOVE_AROUND:
        m_speed = speed_1;
        break;

    default:
        IR_OnKeyboardRelease(key);
        break;
    }
}

void CDemoRecord::IR_OnControllerAttitudeChange(Fvector change)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnControllerAttitudeChange(change);
        return;
    }

    const float scale = 5.f; // psControllerSensorSens;
    OnAxisMove(change.x, change.y, scale, psControllerInvertY.test(1));
}

void CDemoRecord::RecordKey()
{
    Fmatrix g_matView;

    g_matView.invert(m_Camera);
    file->w(&g_matView, sizeof(Fmatrix));
    iCount++;
}

void CDemoRecord::MakeCubemap()
{
    m_bMakeCubeMap = true;
    m_Stage = 0;
}

void CDemoRecord::MakeScreenshot()
{
    m_bMakeScreenshot = true;
    m_Stage = 0;
}

void CDemoRecord::MakeLevelMapScreenshot(bool bHQ)
{
    auto& env = g_pGamePersistent->Environment();
    m_CurrentWeatherCycle = env.CurrentCycleName;
    env.SetWeather("map", true);

    if (!bHQ)
        m_iLMScreenshotFragment = -1;
    else
        m_iLMScreenshotFragment = 0;

    curr_lm_fbox = get_level_screenshot_bound();
    GetLM_BBox(curr_lm_fbox, m_iLMScreenshotFragment);

    m_bMakeLevelMap = true;
    m_Stage = 0;
}

void CDemoRecord::OnRender() { m_Font.OnRender(); }
