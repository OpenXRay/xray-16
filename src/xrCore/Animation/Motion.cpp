#include "stdafx.h"
#pragma hdrstop

#include "Motion.hpp"
#include "xrCore/Animation/Envelope.hpp"
#include "SDL.h"

#define EOBJ_OMOTION 0x1100
#define EOBJ_SMOTION 0x1200
#define EOBJ_OMOTION_VERSION 0x0005
#define EOBJ_SMOTION_VERSION 0x0007

//------------------------------------------------------------------------------------------
// CCustomMotion
//------------------------------------------------------------------------------------------
CCustomMotion::CCustomMotion() : mtype(), iFrameStart(), iFrameEnd(), fFPS(30.) {}

CCustomMotion::CCustomMotion(CCustomMotion* source) { *this = *source; }
CCustomMotion::~CCustomMotion() {}
void CCustomMotion::Save(IWriter& F)
{
    F.w_stringZ(name);
    F.w_u32(iFrameStart);
    F.w_u32(iFrameEnd);
    F.w_float(fFPS);
}

bool CCustomMotion::Load(IReader& F)
{
    F.r_stringZ(name);
    iFrameStart = F.r_u32();
    iFrameEnd = F.r_u32();
    fFPS = F.r_float();
    return true;
}

//------------------------------------------------------------------------------------------
// Object Motion
//------------------------------------------------------------------------------------------
COMotion::COMotion() : CCustomMotion()
{
    mtype = mtObject;
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        envs[ch] = xr_new<CEnvelope>();
}

COMotion::COMotion(COMotion* source) : CCustomMotion(source)
{
    // bone motions
    mtype = source->mtype;
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        envs[ch] = xr_new<CEnvelope>(source->envs[ch]);
}

COMotion::~COMotion() { Clear(); }
void COMotion::Clear()
{
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        xr_delete(envs[ch]);
}

void COMotion::_Evaluate(float t, Fvector& T, Fvector& R)
{
    T.x = envs[ctPositionX]->Evaluate(t);
    T.y = envs[ctPositionY]->Evaluate(t);
    T.z = envs[ctPositionZ]->Evaluate(t);

    R.y = envs[ctRotationH]->Evaluate(t);
    R.x = envs[ctRotationP]->Evaluate(t);
    R.z = envs[ctRotationB]->Evaluate(t);
}

void COMotion::SaveMotion(const char* buf)
{
    CMemoryWriter F;
    F.open_chunk(EOBJ_OMOTION);
    Save(F);
    F.close_chunk();
    if (!F.save_to(buf))
        Log("!Can't save object motion:", buf);
}

bool COMotion::LoadMotion(const char* buf)
{
    destructor<IReader> F(FS.r_open(buf));
    R_ASSERT(F().find_chunk(EOBJ_OMOTION));
    return Load(F());
}

void COMotion::Save(IWriter& F)
{
    CCustomMotion::Save(F);
    F.w_u16(EOBJ_OMOTION_VERSION);
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        envs[ch]->Save(F);
}

bool COMotion::Load(IReader& F)
{
    CCustomMotion::Load(F);
    u16 vers = F.r_u16();
    if (vers == 0x0003)
    {
        Clear();
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
        {
            envs[ch] = xr_new<CEnvelope>();
            envs[ch]->Load_1(F);
        }
    }
    else if (vers == 0x0004)
    {
        Clear();
        envs[ctPositionX] = xr_new<CEnvelope>();
        envs[ctPositionX]->Load_2(F);
        envs[ctPositionY] = xr_new<CEnvelope>();
        envs[ctPositionY]->Load_2(F);
        envs[ctPositionZ] = xr_new<CEnvelope>();
        envs[ctPositionZ]->Load_2(F);
        envs[ctRotationP] = xr_new<CEnvelope>();
        envs[ctRotationP]->Load_2(F);
        envs[ctRotationH] = xr_new<CEnvelope>();
        envs[ctRotationH]->Load_2(F);
        envs[ctRotationB] = xr_new<CEnvelope>();
        envs[ctRotationB]->Load_2(F);
    }
    else
    {
        if (vers != EOBJ_OMOTION_VERSION)
            return false;
        Clear();
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
        {
            envs[ch] = xr_new<CEnvelope>();
            envs[ch]->Load_2(F);
        }
    }
    return true;
}

void COMotion::CreateKey(float t, const Fvector& P, const Fvector& R)
{
    envs[ctPositionX]->InsertKey(t, P.x);
    envs[ctPositionY]->InsertKey(t, P.y);
    envs[ctPositionZ]->InsertKey(t, P.z);
    envs[ctRotationH]->InsertKey(t, R.y);
    envs[ctRotationP]->InsertKey(t, R.x);
    envs[ctRotationB]->InsertKey(t, R.z);
}
void COMotion::DeleteKey(float t)
{
    envs[ctPositionX]->DeleteKey(t);
    envs[ctPositionY]->DeleteKey(t);
    envs[ctPositionZ]->DeleteKey(t);
    envs[ctRotationH]->DeleteKey(t);
    envs[ctRotationP]->DeleteKey(t);
    envs[ctRotationB]->DeleteKey(t);
}
int COMotion::KeyCount() { return envs[ctPositionX]->keys.size(); }
void COMotion::FindNearestKey(float t, float& mn, float& mx, float eps)
{
    KeyIt min_k;
    KeyIt max_k;
    envs[ctPositionX]->FindNearestKey(t, min_k, max_k, eps);
    mn = (min_k != envs[ctPositionX]->keys.end()) ? (*min_k)->time : t;
    mx = (max_k != envs[ctPositionX]->keys.end()) ? (*max_k)->time : t;
}
float COMotion::GetLength(float* mn, float* mx)
{
    float ln, len = 0.f;
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        if ((ln = envs[ch]->GetLength(mn, mx)) > len)
            len = ln;
    return len;
}
BOOL COMotion::ScaleKeys(float from_time, float to_time, float scale_factor)
{
    BOOL bRes = TRUE;
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
        if (FALSE == (bRes = envs[ch]->ScaleKeys(from_time, to_time, scale_factor, 1.f / fFPS)))
            break;
    return bRes;
}
BOOL COMotion::NormalizeKeys(float from_time, float to_time, float speed)
{
    if (to_time < from_time)
        return FALSE;
    CEnvelope* E = Envelope(ctPositionX);
    float new_tm = 0;
    float t0 = E->keys.front()->time;
    FloatVec tms;
    tms.push_back(t0);
    for (KeyIt it = E->keys.begin() + 1; it != E->keys.end(); ++it)
    {
        if ((*it)->time > from_time)
        {
            if ((*it)->time < to_time + EPS)
            {
                float dist = 0;
                Fvector PT, T, R;
                _Evaluate(t0, PT, R);
                for (float tm = t0 + 1.f / fFPS; tm <= (*it)->time; tm += EPS_L)
                {
                    _Evaluate(tm, T, R);
                    dist += PT.distance_to(T);
                    PT.set(T);
                }
                new_tm += dist / speed;
                t0 = (*it)->time;
                tms.push_back(new_tm);
            }
            else
            {
                float dt = (*it)->time - t0;
                t0 = (*it)->time;
                new_tm += dt;
                tms.push_back(new_tm);
            }
        }
    }
    for (size_t ch = 0; ch < ctMaxChannel; ch++)
    {
        E = Envelope(EChannelType(ch));
        FloatIt f_it = tms.begin();
        VERIFY(tms.size() == E->keys.size());
        for (KeyIt k_it = E->keys.begin(); k_it != E->keys.end(); ++k_it, ++f_it)
            (*k_it)->time = *f_it;
    }

    /*
     CEnvelope* E = Envelope();
     for (KeyIt it=E->keys.begin(); it!=E->keys.end(); it++){
     if (((*it)->time>from_time)&&((*it)->time<to_time)){
     for (float tm=from_time; tm<=to_time; tm+=1.f/fFPS){

     }
     }
     */
    return TRUE;
}

CSMotion::CSMotion() : CCustomMotion()
{
    mtype = mtSkeleton;
    m_BoneOrPart = BI_NONE;
    fSpeed = 1.0f;
    fAccrue = 2.0f;
    fFalloff = 2.0f;
    fPower = 1.f;
    m_Flags.zero();
}

CSMotion::CSMotion(CSMotion* source) : CCustomMotion(source)
{
    // bone motions
    st_BoneMotion* src;
    st_BoneMotion* dest;
    for (u32 i = 0; i < bone_mots.size(); i++)
    {
        dest = &bone_mots[i];
        src = &source->bone_mots[i];
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
            dest->envs[ch] = xr_new<CEnvelope>(src->envs[ch]);
    }
}

CSMotion::~CSMotion() { Clear(); }
void CSMotion::Clear()
{
    for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
    {
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
            xr_delete(bm_it->envs[ch]);
    }
    bone_mots.clear();
}

st_BoneMotion* CSMotion::FindBoneMotion(shared_str name)
{
    for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
        if (bm_it->name.equal(name))
            return &*bm_it;
    return 0;
}

void CSMotion::add_empty_motion(shared_str const& bone_id)
{
    VERIFY(!FindBoneMotion(bone_id));

    st_BoneMotion motion;

    motion.SetName(bone_id.c_str());
    // flRKeyAbsent = (1<<1),
    motion.m_Flags.assign(1 << 1);

    for (size_t ch = 0; ch < ctMaxChannel; ch++)
    {
        motion.envs[ch] = xr_new<CEnvelope>();
        // motion.envs[ch];
    }

    bone_mots.push_back(motion);
}

void CSMotion::CopyMotion(CSMotion* source)
{
    Clear();

    iFrameStart = source->iFrameStart;
    iFrameEnd = source->iFrameEnd;
    fFPS = source->fFPS;
    st_BoneMotion* src;
    st_BoneMotion* dest;
    bone_mots.resize(source->bone_mots.size());
    for (size_t i = 0; i < bone_mots.size(); i++)
    {
        dest = &bone_mots[i];
        src = &source->bone_mots[i];
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
            dest->envs[ch] = xr_new<CEnvelope>(src->envs[ch]);
    }
}

void CSMotion::_Evaluate(int bone_idx, float t, Fvector& T, Fvector& R)
{
    VERIFY(bone_idx < (int)bone_mots.size());
    CEnvelope** envs = bone_mots[bone_idx].envs;
    T.x = envs[ctPositionX]->Evaluate(t);
    T.y = envs[ctPositionY]->Evaluate(t);
    T.z = envs[ctPositionZ]->Evaluate(t);

    R.y = envs[ctRotationH]->Evaluate(t);
    R.x = envs[ctRotationP]->Evaluate(t);
    R.z = envs[ctRotationB]->Evaluate(t);
}

void CSMotion::WorldRotate(int boneId, float h, float p, float b)
{
    R_ASSERT((boneId >= 0) && (boneId < (int)bone_mots.size()));
    st_BoneMotion& BM = bone_mots[boneId];

    BM.envs[ctRotationH]->RotateKeys(h);
    BM.envs[ctRotationP]->RotateKeys(p);
    BM.envs[ctRotationB]->RotateKeys(b);
}

void CSMotion::SaveMotion(const char* buf)
{
    CMemoryWriter F;
    F.open_chunk(EOBJ_SMOTION);
    Save(F);
    F.close_chunk();
    if (!F.save_to(buf))
        Log("!Can't save skeleton motion:", buf);
}

bool CSMotion::LoadMotion(const char* buf)
{
    destructor<IReader> F(FS.r_open(buf));
    R_ASSERT(F().find_chunk(EOBJ_SMOTION));
    return Load(F());
}

void CSMotion::Save(IWriter& F)
{
    CCustomMotion::Save(F);
    F.w_u16(EOBJ_SMOTION_VERSION);
    F.w_s8(m_Flags.get());
    F.w_u16(m_BoneOrPart);
    F.w_float(fSpeed);
    F.w_float(fAccrue);
    F.w_float(fFalloff);
    F.w_float(fPower);
    F.w_u16((u16)bone_mots.size());
    for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
    {
        xr_strlwr(bm_it->name);
        F.w_stringZ(bm_it->name);
        F.w_u8(bm_it->m_Flags.get());
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
            bm_it->envs[ch]->Save(F);
    }
    const u32 sz = marks.size();
    F.w_u32(sz);
    for (size_t i = 0; i < sz; ++i)
        marks[i].Save(&F);
}

bool CSMotion::Load(IReader& F)
{
    CCustomMotion::Load(F);
    u16 vers = F.r_u16();
    if (vers == 0x0004)
    {
        m_BoneOrPart = u16(F.r_u32() & 0xffff);
        m_Flags.set(esmFX, F.r_u8());
        m_Flags.set(esmStopAtEnd, F.r_u8());
        fSpeed = F.r_float();
        fAccrue = F.r_float();
        fFalloff = F.r_float();
        fPower = F.r_float();
        bone_mots.resize(F.r_u32());
        string64 temp_buf;
        for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
        {
            bm_it->SetName(xr_itoa(int(bm_it - bone_mots.begin()), temp_buf, 10));
            bm_it->m_Flags.assign((u8)F.r_u32());
            for (size_t ch = 0; ch < ctMaxChannel; ch++)
            {
                bm_it->envs[ch] = xr_new<CEnvelope>();
                bm_it->envs[ch]->Load_1(F);
            }
        }
    }
    else
    {
        if (vers == 0x0005)
        {
            m_Flags.assign((u8)F.r_u32());
            m_BoneOrPart = u16(F.r_u32() & 0xffff);
            fSpeed = F.r_float();
            fAccrue = F.r_float();
            fFalloff = F.r_float();
            fPower = F.r_float();
            bone_mots.resize(F.r_u32());
            string64 buf;
            for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
            {
                F.r_stringZ(buf, sizeof(buf));
                bm_it->SetName(buf);
                bm_it->m_Flags.assign((u8)F.r_u32());
                for (size_t ch = 0; ch < ctMaxChannel; ch++)
                {
                    bm_it->envs[ch] = xr_new<CEnvelope>();
                    bm_it->envs[ch]->Load_1(F);
                }
            }
        }
        else
        {
            if (vers >= 0x0006)
            {
                m_Flags.assign(F.r_u8());
                m_BoneOrPart = F.r_u16();
                fSpeed = F.r_float();
                fAccrue = F.r_float();
                fFalloff = F.r_float();
                fPower = F.r_float();
                bone_mots.resize(F.r_u16());
                string64 buf;
                for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
                {
                    F.r_stringZ(buf, sizeof(buf));
                    bm_it->SetName(buf);
                    bm_it->m_Flags.assign(F.r_u8());
                    for (size_t ch = 0; ch < ctMaxChannel; ch++)
                    {
                        bm_it->envs[ch] = xr_new<CEnvelope>();
                        bm_it->envs[ch]->Load_2(F);
                    }
                }
            }
        }
    }
    if (vers >= 0x0007)
    {
        const size_t sz = F.r_u32();
        if (sz > 0)
        {
            marks.resize(sz);
            for (size_t i = 0; i < sz; ++i)
                marks[i].Load(&F);
        }
    }

    for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
        xr_strlwr(bm_it->name);
    return true;
}

void CSMotion::Optimize()
{
    for (auto bm_it = bone_mots.begin(); bm_it != bone_mots.end(); ++bm_it)
    {
        for (size_t ch = 0; ch < ctMaxChannel; ch++)
            bm_it->envs[ch]->Optimize();
    }
}

void CSMotion::SortBonesBySkeleton(BoneVec& bones)
{
    BoneMotionVec new_bone_mots;
    for (auto b_it = bones.begin(); b_it != bones.end(); ++b_it)
    {
        st_BoneMotion* BM = FindBoneMotion((*b_it)->Name());
        // previously there was R_ASSERT(BM) (for use with plugins only)
        if (!BM)
        {
            CBone* B = *(b_it);
            bone_mots.push_back(st_BoneMotion());
            st_BoneMotion& bm0 = bone_mots[0];
            st_BoneMotion& bm = bone_mots.back();
            bm.SetName(B->Name().c_str());
            bm.m_Flags.assign(bm0.m_Flags);

            for (size_t ch = 0; ch < ctMaxChannel; ++ch)
            {
                bm.envs[ch] = xr_new<CEnvelope>();
                //. bm.envs[ch]->Load_2(F);
            }
            bm.envs[ctPositionX]->InsertKey(0.0f, B->Offset().x);
            bm.envs[ctPositionY]->InsertKey(0.0f, B->Offset().y);
            bm.envs[ctPositionZ]->InsertKey(0.0f, B->Offset().z);
            bm.envs[ctRotationH]->InsertKey(0.0f, B->Rotate().x);
            bm.envs[ctRotationP]->InsertKey(0.0f, B->Rotate().y);
            bm.envs[ctRotationB]->InsertKey(0.0f, B->Rotate().z);
            BM = &bm;
        };
        new_bone_mots.push_back(*BM);
    }
    bone_mots.clear();
    bone_mots = std::move(new_bone_mots);
}

void SAnimParams::Set(float start_frame, float end_frame, float fps)
{
    min_t = start_frame / fps;
    max_t = end_frame / fps;
}

void SAnimParams::Set(CCustomMotion* M)
{
    Set((float)M->FrameStart(), (float)M->FrameEnd(), M->FPS());
    t_current = min_t;
    tmp = t_current;
    // bPlay=true;
}
void SAnimParams::Update(float dt, float speed, bool loop)
{
    if (!bPlay)
        return;
    bWrapped = false;

    t_current += speed * dt;
    tmp = t_current;

    if (t_current > max_t)
    {
        bWrapped = true;
        if (loop)
        {
            float len = max_t - min_t;
            float k = float(iFloor((t_current - min_t) / len));
            t_current = t_current - k * len;
        }
        else
            t_current = max_t;

        tmp = t_current;
    }
}

//------------------------------------------------------------------------------
// Clip
//------------------------------------------------------------------------------
#define EOBJ_CLIP_VERSION 2
#define EOBJ_CLIP_VERSION_CHUNK 0x9000
#define EOBJ_CLIP_DATA_CHUNK 0x9001

void CClip::Save(IWriter& F)
{
    F.open_chunk(EOBJ_CLIP_VERSION_CHUNK);
    F.w_u16(EOBJ_CLIP_VERSION);
    F.close_chunk();

    F.open_chunk(EOBJ_CLIP_DATA_CHUNK);
    F.w_stringZ(name);
    for (size_t k = 0; k < 4; k++)
    {
        F.w_stringZ(cycles[k].name);
        F.w_u16(cycles[k].slot);
    }
    F.w_stringZ(fx.name);
    F.w_u16(fx.slot);
    F.w_float(fx_power);
    F.w_float(length);
    F.close_chunk();
}
//------------------------------------------------------------------------------

bool CClip::Load(IReader& F)
{
    R_ASSERT(F.find_chunk(EOBJ_CLIP_VERSION_CHUNK));
    u16 ver = F.r_u16();
    if (ver != EOBJ_CLIP_VERSION)
        return false;
    R_ASSERT(F.find_chunk(EOBJ_CLIP_DATA_CHUNK));
    F.r_stringZ(name);
    for (size_t k = 0; k < 4; k++)
    {
        F.r_stringZ(cycles[k].name);
        cycles[k].slot = F.r_u16();
    }
    F.r_stringZ(fx.name);
    fx.slot = F.r_u16();
    fx_power = F.r_float();
    length = F.r_float();
    return true;
}
//------------------------------------------------------------------------------

bool CClip::Equal(CClip* c)
{
    if (!name.equal(c->name))
        return false;
    if (!cycles[0].equal(c->cycles[0]))
        return false;
    if (!cycles[1].equal(c->cycles[1]))
        return false;
    if (!cycles[2].equal(c->cycles[2]))
        return false;
    if (!cycles[3].equal(c->cycles[3]))
        return false;
    if (!fx.equal(c->fx))
        return false;
    if (length != c->length)
        return false;
    return true;
}
//------------------------------------------------------------------------------
