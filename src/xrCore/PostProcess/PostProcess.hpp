#pragma once
#include "xrCore/xrCore.h"
#include "xrCore/Animation/Envelope.hpp"
#include "xrCore/PostProcess/PPInfo.hpp"

#define POSTPROCESS_PARAMS_COUNT 11
//.#define POSTPROCESS_FILE_VERSION 0x0001
#define POSTPROCESS_FILE_VERSION 0x0002
constexpr pcstr POSTPROCESS_FILE_EXTENSION = ".ppe";

typedef enum _pp_params {
    pp_unknown = -1,
    pp_base_color = 0,
    pp_add_color = 1,
    pp_gray_color = 2,
    pp_gray_value = 3,
    pp_blur = 4,
    pp_dual_h = 5,
    pp_dual_v = 6,
    pp_noise_i = 7,
    pp_noise_g = 8,
    pp_noise_f = 9,
    pp_cm_influence = 10,
    pp_last = 11,
    pp_force_dword = 0x7fffffff
} pp_params;

class XRCORE_API CPostProcessParam
{
protected:
public:
    virtual void update(float dt) = 0;
    virtual void load(IReader& pReader) = 0;
    virtual void save(IWriter& pWriter) = 0;
    virtual float get_length() = 0;
    virtual size_t get_keys_count() = 0;
    virtual ~CPostProcessParam() {}
    virtual void add_value(float time, float value, int index = 0) = 0;
    virtual void delete_value(float time) = 0;
    virtual void update_value(float time, float value, int index = 0) = 0;
    virtual void get_value(float time, float& value, int index = 0) = 0;
    virtual float get_key_time(size_t index) = 0;
    virtual void clear_all_keys() = 0;
};

class XRCORE_API CPostProcessValue final : public CPostProcessParam
{
protected:
    CEnvelope m_Value;
    float* m_pfParam;

public:
    CPostProcessValue(float* pfparam) : m_pfParam(pfparam) {}
    void update(float dt) override { *m_pfParam = m_Value.Evaluate(dt); }
    void load(IReader& pReader) override;
    void save(IWriter& pWriter) override;

    float get_length() override
    {
        float mn, mx;
        return m_Value.GetLength(&mn, &mx);
    }

    size_t get_keys_count() override { return m_Value.keys.size(); }
    void add_value(float time, float value, int index = 0) override;
    void delete_value(float time) override;
    void update_value(float time, float value, int index = 0) override;
    void get_value(float time, float& valueb, int index = 0) override;

    float get_key_time(size_t index) override
    {
        VERIFY(index < get_keys_count());
        return m_Value.keys[index]->time;
    }

    void clear_all_keys() override;
};

class XRCORE_API CPostProcessColor final : public CPostProcessParam
{
protected:
    float m_fBase;
    SPPInfo::SColor* m_pColor;
    CEnvelope m_Red;
    CEnvelope m_Green;
    CEnvelope m_Blue;

public:
    CPostProcessColor(SPPInfo::SColor* pcolor) : m_fBase(0.0),  m_pColor(pcolor) {}

    void update(float dt) override
    {
        m_pColor->r = m_Red.Evaluate(dt);
        m_pColor->g = m_Green.Evaluate(dt);
        m_pColor->b = m_Blue.Evaluate(dt);
    }

    void load(IReader& pReader) override;
    void save(IWriter& pWriter) override;

    float get_length() override
    {
        float mn, mx, r = m_Red.GetLength(&mn, &mx), g = m_Green.GetLength(&mn, &mx), b = m_Blue.GetLength(&mn, &mx);
        mn = (r > g ? r : g);
        return mn > b ? mn : b;
    }

    size_t get_keys_count() override { return m_Red.keys.size(); }
    void add_value(float time, float value, int index = 0) override;
    void delete_value(float time) override;
    void update_value(float time, float value, int index = 0) override;
    void get_value(float time, float& value, int index = 0) override;

    float get_key_time(size_t index) override
    {
        VERIFY(index < get_keys_count());
        return m_Red.keys[index]->time;
    }

    void clear_all_keys() override;
};

class XRCORE_API BasicPostProcessAnimator
{
protected:
    SPPInfo m_EffectorParams;
    CPostProcessParam* m_Params[POSTPROCESS_PARAMS_COUNT];
    shared_str m_Name;
    float m_factor;
    float m_dest_factor;
    bool m_bStop;
    float m_factor_speed;
    bool m_bCyclic;
    float m_start_time;
    float f_length;

protected:
    void Update(float tm);

public:
    BasicPostProcessAnimator(int id, bool cyclic);
    BasicPostProcessAnimator();
    virtual ~BasicPostProcessAnimator();
    void Clear();
    virtual void Load(LPCSTR name, bool internalFs = true);
    IC LPCSTR Name() { return *m_Name; }
    virtual void Stop(float speed);
    void SetDesiredFactor(float f, float sp);
    void SetCurrentFactor(float f);
    void SetCyclic(bool b) { m_bCyclic = b; }
    float GetLength();
    SPPInfo& PPinfo() { return m_EffectorParams; }
    virtual BOOL Process(float dt, SPPInfo& PPInfo);
    void Create();
    CPostProcessParam* GetParam(pp_params param);
    void ResetParam(pp_params param);
    void Save(LPCSTR name);
};
