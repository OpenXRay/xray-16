#include "stdafx.h"
#include "IGame_Level.h"
#include "x_ray.h"

#include "GameFont.h"
#include "FDemoRecord.h"
#include "XR_IOConsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "Render.h"
#include "CustomHUD.h"
#include "CameraManager.h"

extern BOOL g_bDisableRedText;
static Flags32 s_hud_flag = {0};
static Flags32 s_dev_flags = {0};

BOOL stored_weapon;
BOOL stored_cross;
BOOL stored_red_text;

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
void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags);
CDemoRecord::CDemoRecord(const char* name, float life_time) : CEffectorCam(cefDemo, life_time /*,FALSE*/)
{
    stored_red_text = g_bDisableRedText;
    g_bDisableRedText = TRUE;
    m_iLMScreenshotFragment = -1;
    /*
     stored_weapon = psHUD_Flags.test(HUD_WEAPON);
     stored_cross = psHUD_Flags.test(HUD_CROSSHAIR);
     psHUD_Flags.set(HUD_WEAPON, FALSE);
     psHUD_Flags.set(HUD_CROSSHAIR, FALSE);
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
        m_bMakeCubeMap = FALSE;
        m_bMakeScreenshot = FALSE;
        m_bMakeLevelMap = FALSE;

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
        m_bMakeScreenshot = FALSE;
        break;
    }
    m_Stage++;
}

void GetLM_BBox(Fbox& bb, INT Step)
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
        s_dev_flags = psDeviceFlags;
        s_hud_flag.assign(psHUD_Flags);
        psDeviceFlags.zero();
        psDeviceFlags.set(rsClearBB | rsFullscreen | rsDrawStatic, TRUE);
        if (!psDeviceFlags.equal(s_dev_flags, rsFullscreen))
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

            BOOL bDevReset = !psDeviceFlags.equal(s_dev_flags, rsFullscreen);
            psDeviceFlags = s_dev_flags;
            if (bDevReset)
                Device.Reset();
            m_bMakeLevelMap = FALSE;
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
        m_bMakeCubeMap = FALSE;
        break;
    }
    m_Stage++;
}

BOOL CDemoRecord::ProcessCam(SCamEffectorInfo& info)
{
    info.dont_apply = false;
    if (0 == file)
        return TRUE;

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
        if (IR_GetKeyState(DIK_F1))
        {
            pApp->pFontSystem->SetColor(color_rgba(255, 0, 0, 255));
            pApp->pFontSystem->SetAligment(CGameFont::alCenter);
            pApp->pFontSystem->OutSetI(0, -.05f);
            pApp->pFontSystem->OutNext("%s", "RECORDING");
            pApp->pFontSystem->OutNext("Key frames count: %d", iCount);
            pApp->pFontSystem->SetAligment(CGameFont::alLeft);
            pApp->pFontSystem->OutSetI(-0.2f, +.05f);
            pApp->pFontSystem->OutNext("SPACE");
            pApp->pFontSystem->OutNext("BACK");
            pApp->pFontSystem->OutNext("ESC");
            pApp->pFontSystem->OutNext("F11");
            pApp->pFontSystem->OutNext("LCONTROL+F11");
            pApp->pFontSystem->OutNext("F12");
            pApp->pFontSystem->SetAligment(CGameFont::alLeft);
            pApp->pFontSystem->OutSetI(0, +.05f);
            pApp->pFontSystem->OutNext("= Append Key");
            pApp->pFontSystem->OutNext("= Cube Map");
            pApp->pFontSystem->OutNext("= Quit");
            pApp->pFontSystem->OutNext("= Level Map ScreenShot");
            pApp->pFontSystem->OutNext("= Level Map ScreenShot(High Quality)");
            pApp->pFontSystem->OutNext("= ScreenShot");
        }

        m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
        m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

        float speed = m_fSpeed1, ang_speed = m_fAngSpeed1;

        if (IR_GetKeyState(DIK_LSHIFT))
        {
            speed = m_fSpeed0;
            ang_speed = m_fAngSpeed0;
        }
        else if (IR_GetKeyState(DIK_LALT))
        {
            speed = m_fSpeed2;
            ang_speed = m_fAngSpeed2;
        }
        else if (IR_GetKeyState(DIK_LCONTROL))
        {
            speed = m_fSpeed3;
            ang_speed = m_fAngSpeed3;
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
    return TRUE;
}

void CDemoRecord::IR_OnKeyboardPress(int dik)
{
    if (dik == DIK_MULTIPLY)
        m_b_redirect_input_to_level = !m_b_redirect_input_to_level;

    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnKeyboardPress(dik);
        return;
    }
    if (dik == DIK_GRAVE)
        Console->Show();
    if (dik == DIK_SPACE)
        RecordKey();
    if (dik == DIK_BACK)
        MakeCubemap();
    if (dik == DIK_F11)
        MakeLevelMapScreenshot(IR_GetKeyState(DIK_LCONTROL));
    if (dik == DIK_F12)
        MakeScreenshot();
    if (dik == DIK_ESCAPE)
        fLifeTime = -1;

#ifdef DEBUG // ndef MASTER_GOLD // Xottab_DUTY: Teleport to demo cam in Debug configuration
    if (dik == DIK_RETURN)
    {
        if (g_pGameLevel->CurrentEntity())
        {
            g_pGameLevel->CurrentEntity()->ForceTransform(m_Camera);
            fLifeTime = -1;
        }
    }
#endif

    if (dik == DIK_PAUSE)
        Device.Pause(!Device.Paused(), TRUE, TRUE, "demo_record");
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
    case DIK_A:
    case DIK_NUMPAD1:
    case DIK_LEFT:
        vT_delta.x -= 1.0f;
        break; // Slide Left
    case DIK_D:
    case DIK_NUMPAD3:
    case DIK_RIGHT:
        vT_delta.x += 1.0f;
        break; // Slide Right
    case DIK_S:
        vT_delta.y -= 1.0f;
        break; // Slide Down
    case DIK_W:
        vT_delta.y += 1.0f;
        break; // Slide Up
    // rotate
    case DIK_NUMPAD2:
        vR_delta.x -= 1.0f;
        break; // Pitch Down
    case DIK_NUMPAD8:
        vR_delta.x += 1.0f;
        break; // Pitch Up
    case DIK_E:
    case DIK_NUMPAD6:
        vR_delta.y += 1.0f;
        break; // Turn Left
    case DIK_Q:
    case DIK_NUMPAD4:
        vR_delta.y -= 1.0f;
        break; // Turn Right
    case DIK_NUMPAD9:
        vR_delta.z -= 2.0f;
        break; // Turn Right
    case DIK_NUMPAD7:
        vR_delta.z += 2.0f;
        break; // Turn Right
    }

    update_whith_timescale(m_vT, vT_delta);
    update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnMouseMove(dx, dy);
        return;
    }

    Fvector vR_delta = Fvector().set(0, 0, 0);

    float scale = .5f; // psMouseSens;
    if (dx || dy)
    {
        vR_delta.y += float(dx) * scale; // heading
        vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
    }
    update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
    if (m_b_redirect_input_to_level)
    {
        g_pGameLevel->IR_OnMouseHold(btn);
        return;
    }
    Fvector vT_delta = Fvector().set(0, 0, 0);
    switch (btn)
    {
    case 0:
        vT_delta.z += 1.0f;
        break; // Move Backward
    case 1:
        vT_delta.z -= 1.0f;
        break; // Move Forward
    }
    update_whith_timescale(m_vT, vT_delta);
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
    m_bMakeCubeMap = TRUE;
    m_Stage = 0;
}

void CDemoRecord::MakeScreenshot()
{
    m_bMakeScreenshot = TRUE;
    m_Stage = 0;
}

void CDemoRecord::MakeLevelMapScreenshot(BOOL bHQ)
{
    Console->Execute("run_string level.set_weather(\"map\",true)");

    if (!bHQ)
        m_iLMScreenshotFragment = -1;
    else
        m_iLMScreenshotFragment = 0;

    curr_lm_fbox = get_level_screenshot_bound();
    GetLM_BBox(curr_lm_fbox, m_iLMScreenshotFragment);

    m_bMakeLevelMap = TRUE;
    m_Stage = 0;
}

void CDemoRecord::OnRender() { pApp->pFontSystem->OnRender(); }
