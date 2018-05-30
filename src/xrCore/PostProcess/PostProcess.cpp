#include "stdafx.h"
#include "PostProcess.hpp"

// postprocess value LOAD method implementation
void CPostProcessValue::load(IReader& pReader) { m_Value.Load_2(pReader); }
void CPostProcessValue::save(IWriter& pWriter) { m_Value.Save(pWriter); }
// postprocess color LOAD method implementation
void CPostProcessColor::load(IReader& pReader)
{
    m_fBase = pReader.r_float();
    m_Red.Load_2(pReader);
    m_Green.Load_2(pReader);
    m_Blue.Load_2(pReader);
}

void CPostProcessColor::save(IWriter& pWriter)
{
    pWriter.w_float(m_fBase);
    m_Red.Save(pWriter);
    m_Green.Save(pWriter);
    m_Blue.Save(pWriter);
}

// main PostProcessAnimator class

BasicPostProcessAnimator::BasicPostProcessAnimator() { Create(); }
BasicPostProcessAnimator::BasicPostProcessAnimator(int id, bool cyclic) : m_bCyclic(cyclic) { Create(); }
BasicPostProcessAnimator::~BasicPostProcessAnimator() { Clear(); }
void BasicPostProcessAnimator::Clear()
{
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
        xr_delete(m_Params[a]);
}

void BasicPostProcessAnimator::Load(LPCSTR name, bool internalFs /*= true*/)
{
    m_Name = name;
    string_path full_path;
    if (internalFs)
    {
        if (!FS.exist(full_path, "$level$", name))
        {
            if (!FS.exist(full_path, "$game_anims$", name))
                xrDebug::Fatal(DEBUG_INFO, "Can't find motion file '%s'.", name);
        }
    }
    else
        xr_strcpy(full_path, name);

    LPCSTR ext = strext(full_path);
    if (ext)
    {
        if (!xr_strcmp(ext, POSTPROCESS_FILE_EXTENSION))
        {
            IReader* F = FS.r_open(full_path);
            u32 dwVersion = F->r_u32();
            //.       VERIFY (dwVersion == POSTPROCESS_FILE_VERSION);
            // load base color
            VERIFY(m_Params[0]);
            m_Params[0]->load(*F);
            // load add color
            VERIFY(m_Params[1]);
            m_Params[1]->load(*F);
            // load gray color
            VERIFY(m_Params[2]);
            m_Params[2]->load(*F);
            // load gray value
            VERIFY(m_Params[3]);
            m_Params[3]->load(*F);
            // load blur value
            VERIFY(m_Params[4]);
            m_Params[4]->load(*F);
            // load duality horizontal
            VERIFY(m_Params[5]);
            m_Params[5]->load(*F);
            // load duality vertical
            VERIFY(m_Params[6]);
            m_Params[6]->load(*F);
            // load noise intensity
            VERIFY(m_Params[7]);
            m_Params[7]->load(*F);
            // load noise granularity
            VERIFY(m_Params[8]);
            m_Params[8]->load(*F);
            // load noise fps
            VERIFY(m_Params[9]);
            m_Params[9]->load(*F);
            if (dwVersion >= 0x0002)
            {
                VERIFY(m_Params[10]);
                m_Params[10]->load(*F);
                F->r_stringZ(m_EffectorParams.cm_tex1);
            }
            // close reader
            FS.r_close(F);
        }
        else
            FATAL("ERROR: Can't support files with many animations set. Incorrect file.");
    }

    f_length = GetLength();
}

void BasicPostProcessAnimator::Stop(float sp)
{
    if (m_bStop)
        return;
    m_bStop = true;
    VERIFY(_valid(sp));
    m_factor_speed = sp;
}

float BasicPostProcessAnimator::GetLength()
{
    float v = 0.0f;
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
    {
        float t = m_Params[a]->get_length();
        v = _max(t, v);
    }
    return v;
}

void BasicPostProcessAnimator::Update(float tm)
{
    for (int a = 0; a < POSTPROCESS_PARAMS_COUNT; a++)
        m_Params[a]->update(tm);
}

void BasicPostProcessAnimator::SetDesiredFactor(float f, float sp)
{
    m_dest_factor = f;
    m_factor_speed = sp;
    VERIFY(_valid(m_factor));
    VERIFY(_valid(m_dest_factor));
};

void BasicPostProcessAnimator::SetCurrentFactor(float f)
{
    m_factor = f;
    m_dest_factor = f;
    VERIFY(_valid(m_factor));
    VERIFY(_valid(m_dest_factor));
};

BOOL BasicPostProcessAnimator::Process(float dt, SPPInfo& PPInfo)
{
    Update(dt);

    // if(m_bStop)
    //		m_factor			-=	dt*m_stop_speed;

    clamp(m_factor, 0.001f, 1.0f);

    PPInfo = m_EffectorParams;
    //	PPInfo.lerp				(pp_identity, m_EffectorParams, m_factor);

    //	if(fsimilar(m_factor,0.001f,EPS_S))
    //		return FALSE;

    return TRUE;
}

void BasicPostProcessAnimator::Create()
{
    m_factor = 1.0f;
    m_dest_factor = 1.0f;
    m_bStop = false;
    m_start_time = -1.0f;
    m_factor_speed = 1.0f;
    f_length = 0.0f;

    m_Params[0] = new CPostProcessColor(&m_EffectorParams.color_base); // base color
    VERIFY(m_Params[0]);
    m_Params[1] = new CPostProcessColor(&m_EffectorParams.color_add); // add color
    VERIFY(m_Params[1]);
    m_Params[2] = new CPostProcessColor(&m_EffectorParams.color_gray); // gray color
    VERIFY(m_Params[2]);
    m_Params[3] = new CPostProcessValue(&m_EffectorParams.gray); // gray value
    VERIFY(m_Params[3]);
    m_Params[4] = new CPostProcessValue(&m_EffectorParams.blur); // blur value
    VERIFY(m_Params[4]);
    m_Params[5] = new CPostProcessValue(&m_EffectorParams.duality.h); // duality horizontal
    VERIFY(m_Params[5]);
    m_Params[6] = new CPostProcessValue(&m_EffectorParams.duality.v); // duality vertical
    VERIFY(m_Params[6]);
    m_Params[7] = new CPostProcessValue(&m_EffectorParams.noise.intensity); // noise intensity
    VERIFY(m_Params[7]);
    m_Params[8] = new CPostProcessValue(&m_EffectorParams.noise.grain); // noise granularity
    VERIFY(m_Params[8]);
    m_Params[9] = new CPostProcessValue(&m_EffectorParams.noise.fps); // noise fps
    VERIFY(m_Params[9]);
    m_Params[10] = new CPostProcessValue(&m_EffectorParams.cm_influence);
    VERIFY(m_Params[10]);
}

CPostProcessParam* BasicPostProcessAnimator::GetParam(pp_params param)
{
    VERIFY(param >= pp_base_color && param < pp_last);
    return m_Params[param];
}

void BasicPostProcessAnimator::Save(LPCSTR name)
{
    IWriter* W = FS.w_open(name);
    VERIFY(W);
    W->w_u32(POSTPROCESS_FILE_VERSION);
    m_Params[0]->save(*W);
    m_Params[1]->save(*W);
    m_Params[2]->save(*W);
    m_Params[3]->save(*W);
    m_Params[4]->save(*W);
    m_Params[5]->save(*W);
    m_Params[6]->save(*W);
    m_Params[7]->save(*W);
    m_Params[8]->save(*W);
    m_Params[9]->save(*W);
    m_Params[10]->save(*W);
    W->w_stringZ(m_EffectorParams.cm_tex1);
    FS.w_close(W);
}

void BasicPostProcessAnimator::ResetParam(pp_params param)
{
    xr_delete(m_Params[param]);
    switch (param)
    {
    case pp_base_color:
        m_Params[0] = new CPostProcessColor(&m_EffectorParams.color_base); // base color
        break;
    case pp_add_color:
        m_Params[1] = new CPostProcessColor(&m_EffectorParams.color_add); // add color
        break;
    case pp_gray_color:
        m_Params[2] = new CPostProcessColor(&m_EffectorParams.color_gray); // gray color
        break;
    case pp_gray_value:
        m_Params[3] = new CPostProcessValue(&m_EffectorParams.gray); // gray value
        break;
    case pp_blur:
        m_Params[4] = new CPostProcessValue(&m_EffectorParams.blur); // blur value
        break;
    case pp_dual_h:
        m_Params[5] = new CPostProcessValue(&m_EffectorParams.duality.h); // duality horizontal
        break;
    case pp_dual_v:
        m_Params[6] = new CPostProcessValue(&m_EffectorParams.duality.v); // duality vertical
        break;
    case pp_noise_i:
        m_Params[7] = new CPostProcessValue(&m_EffectorParams.noise.intensity); // noise intensity
        break;
    case pp_noise_g:
        m_Params[8] = new CPostProcessValue(&m_EffectorParams.noise.grain); // noise granularity
        break;
    case pp_noise_f:
        m_Params[9] = new CPostProcessValue(&m_EffectorParams.noise.fps); // noise fps
        break;
    case pp_cm_influence: m_Params[10] = new CPostProcessValue(&m_EffectorParams.cm_influence); break;
    }
    VERIFY(m_Params[param]);
}

void CPostProcessColor::clear_all_keys()
{
    m_Red.ClearAndFree();
    m_Green.ClearAndFree();
    m_Blue.ClearAndFree();
}

void CPostProcessColor::delete_value(float time)
{
    m_Red.DeleteKey(time);
    m_Green.DeleteKey(time);
    m_Blue.DeleteKey(time);
}

void CPostProcessColor::add_value(float time, float value, int index)
{
    KeyIt i;
    if (0 == index)
    {
        m_Red.InsertKey(time, value);
        i = m_Red.FindKey(time, 0.01f);
    }
    else if (1 == index)
    {
        m_Green.InsertKey(time, value);
        i = m_Green.FindKey(time, 0.01f);
    }
    else
    {
        m_Blue.InsertKey(time, value);
        i = m_Blue.FindKey(time, 0.01f);
    }
    (*i)->tension = 0;
    (*i)->continuity = 0;
    (*i)->bias = 0;
}

void CPostProcessColor::update_value(float time, float value, int index)
{
    KeyIt i;
    if (0 == index)
        i = m_Red.FindKey(time, 0.01f);
    else if (1 == index)
        i = m_Green.FindKey(time, 0.01f);
    else
        i = m_Blue.FindKey(time, 0.01f);

    (*i)->value = value;
    (*i)->tension = 0;
    (*i)->continuity = 0;
    (*i)->bias = 0;
}

void CPostProcessColor::get_value(float time, float& value, int index)
{
    KeyIt i;
    if (0 == index)
        i = m_Red.FindKey(time, 0.01f);
    else if (1 == index)
        i = m_Green.FindKey(time, 0.01f);
    else
        i = m_Blue.FindKey(time, 0.01f);
    value = (*i)->value;
}

void CPostProcessValue::delete_value(float time) { m_Value.DeleteKey(time); }
void CPostProcessValue::clear_all_keys() { m_Value.ClearAndFree(); }
void CPostProcessValue::add_value(float time, float value, int index)
{
    m_Value.InsertKey(time, value);
    KeyIt i = m_Value.FindKey(time, 0.01f);

    (*i)->tension = 0;
    (*i)->continuity = 0;
    (*i)->bias = 0;
}

void CPostProcessValue::update_value(float time, float value, int index)
{
    KeyIt i = m_Value.FindKey(time, 0.01f);
    (*i)->value = value;
    (*i)->tension = 0;
    (*i)->continuity = 0;
    (*i)->bias = 0;
}

void CPostProcessValue::get_value(float time, float& value, int index)
{
    KeyIt i = m_Value.FindKey(time, 0.01f);
    value = (*i)->value;
}
