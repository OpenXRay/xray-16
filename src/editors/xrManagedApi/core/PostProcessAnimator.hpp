#include "core/fs/FS.hpp"
#include "xrCore/PostProcess/PostProcess.hpp"

namespace XRay
{
namespace ManagedApi
{
namespace Core
{

using namespace System::Runtime::InteropServices;

// pp_params
public enum class PostProcessParamType : int
{
    Unknown = pp_params::pp_unknown,
    BaseColor = pp_params::pp_base_color,
    AddColor = pp_params::pp_add_color,
    GrayColor = pp_params::pp_gray_color,
    GrayValue = pp_params::pp_gray_value,
    Blur = pp_params::pp_blur,
    DualityH = pp_params::pp_dual_h,
    DualityV = pp_params::pp_dual_v,
    NoiseIntensity = pp_params::pp_noise_i,
    NoiseGrain = pp_params::pp_noise_g,
    NoiseFps = pp_params::pp_noise_f,
    ColorMappingInfluence = pp_params::pp_cm_influence,
    LastValue = pp_params::pp_last,
    ForceDword = pp_params::pp_force_dword,
};

// CPostProcessParam
public ref class PostProcessParamBase abstract
{
internal:
    ::CPostProcessParam* impl;
    bool dontDestroy = false;
    PostProcessParamBase(::CPostProcessParam* impl);
public:
    ~PostProcessParamBase();
    // update
    virtual void Update(float dt) = 0;
    // load(IReader)
    virtual void Load(ReaderBase^ reader) = 0;
    // save(IWriter)
    virtual void Save(WriterBase^ writer) = 0;
    // get_length
    virtual property float Length { float get() = 0; }
    // get_keys_count
    virtual property int KeyCount { int get() = 0; }
    // add_value
    virtual void AddValue(float time, float value, int index) = 0;
    // delete_value
    virtual void DeleteValue(float time) = 0;
    // update_value
    virtual void UpdateValue(float time, float value, int index) = 0;
    // get_value
    virtual void GetValue(float time, [Out] float% value, int index) = 0;
    // get_key_time
    virtual float GetKeyTime(int index) = 0;
    // clear_all_keys
    virtual void Reset() = 0;
};

// CPostProcessValue
public ref class PostProcessParam : PostProcessParamBase
{
internal:
    PostProcessParam(::CPostProcessValue* impl);
public:
    // update
    virtual void Update(float dt) override;
    // load(IReader)
    virtual void Load(ReaderBase^ reader) override;
    // save(IWriter)
    virtual void Save(WriterBase^ writer) override;
    // get_length
    virtual property float Length { float get() override; }
    // get_keys_count
    virtual property int KeyCount { int get() override; }
    // add_value
    virtual void AddValue(float time, float value, int index) override;
    // delete_value
    virtual void DeleteValue(float time) override;
    // update_value
    virtual void UpdateValue(float time, float value, int index) override;
    // get_value
    virtual void GetValue(float time, [Out] float% value, int index) override;
    // get_key_time
    virtual float GetKeyTime(int index) override;
    // clear_all_keys
    virtual void Reset() override;
};

// SPPInfo
public ref class PostProcessInfo
{
public:
    [StructLayout(LayoutKind::Explicit, Size = sizeof(Fvector3))]
    value struct Color
    {
    private:
        [FieldOffset(0)]
        Vector3F vec;
    public:
        [FieldOffset(0)]
        float r;
        [FieldOffset(1 * sizeof(float))]
        float g;
        [FieldOffset(2 * sizeof(float))]
        float b;

        Color(float r, float g, float b) : r(r), g(g), b(b) {}
        operator UInt32()
        {
            int _r = clampr(iFloor(r*255.0f + 0.5f), 0, 255);
            int _g = clampr(iFloor(g*255.0f + 0.5f), 0, 255);
            int _b = clampr(iFloor(b*255.0f + 0.5f), 0, 255);
            return color_rgba(_r, _g, _b, 0);
        }
        operator const Vector3F % () { return vec; }
        Color% operator += (const Color% ppi)
        {
            r += ppi.r;
            g += ppi.g;
            b += ppi.b;
            return *this;
        }
        Color% operator -= (const Color% ppi)
        {
            r -= ppi.r;
            g -= ppi.g;
            b -= ppi.b;
            return *this;
        }
        Color% set(float _r, float _g, float _b)
        {
            r = _r;
            g = _g;
            b = _b;
            return *this;
        }
    };
    value struct Duality
    {
    public:
        float h, v;

        Duality(float _h, float _v) :h(_h), v(_v) {}
        Duality% set(float _h, float _v)
        {
            h = _h;
            v = _v;
            return *this;
        }
    };
    value struct Noise
    {
    public:
        float intensity, grain, fps;

        Noise(float _i, float _g, float _f) : intensity(_i), grain(_g), fps(_f) {}
        Noise% set(float _i, float _g, float _f)
        {
            intensity = _i;
            grain = _g;
            fps = _f;
            return *this;
        }
    };
internal:
    ::SPPInfo* impl;
    bool dontDestroy = false;
public:
    // blur
    property float Blur { float get(); void set(float value); }
    // gray
    property float Gray { float get(); void set(float value); }
    // color_base
    property Color BaseColor { Color get(); void set(Color value); }
    // color_gray
    property Color GrayColor { Color get(); void set(Color value); }
    // color_add
    property Color AddColor { Color get(); void set(Color value); }
    // cm_influence
    property float ColorMappingInfluence { float get(); void set(float value); }
    // cm_interpolate
    property float ColorMappingInterpolate { float get(); void set(float value); }
    // cm_tex1
    property String^ ColorMappingGradient1 { String^ get(); void set(String^ value); }
    // cm_tex2
    property String^ ColorMappingGradient2 { String^ get(); void set(String^ value); }

    PostProcessInfo(::SPPInfo* impl);
    PostProcessInfo(::SPPInfo* impl, bool dontDestroy);
    ~PostProcessInfo();
    // add
    PostProcessInfo% Add(const PostProcessInfo% ppi);
    // sub
    PostProcessInfo% Substract(const PostProcessInfo% ppi);
    // normalize
    void Normalize();
    // lerp
    PostProcessInfo% Interpolate(const PostProcessInfo% def, const PostProcessInfo% to, float factor);
    // validate
    void Validate(String^ str);
};

ref class PostProcessParamProxy : PostProcessParamBase
{
public:
    PostProcessParamProxy(::CPostProcessParam* impl);
    virtual void Update(float dt) override;
    virtual void Load(ReaderBase^ reader) override;
    virtual void Save(WriterBase^ writer) override;
    virtual property float Length { float get() override; }
    virtual property int KeyCount { int get() override; }
    virtual void AddValue(float time, float value, int index) override;
    virtual void DeleteValue(float time) override;
    virtual void UpdateValue(float time, float value, int index) override;
    virtual void GetValue(float time, [Out] float% value, int index) override;
    virtual float GetKeyTime(int index) override;
    virtual void Reset() override;
};

// BasicPostProcessAnimator (CPostprocessAnimator)
public ref class BasicPostProcessAnimator
{
internal:
    ::BasicPostProcessAnimator* impl;
public:
    BasicPostProcessAnimator();
    BasicPostProcessAnimator(int id, bool cyclic);
    ~BasicPostProcessAnimator();
    void Clear();
    // set internalFs to false to load from arbitrary directory
    void Load(String^ name, bool internalFs);
    property String^ Name { String^ get(); }
    virtual void Stop(float speed);
    void SetDesiredFactor(float f, float sp);
    void SetCurrentFactor(float f);
    void SetCyclic(bool b);
    // GetLength
    property float Length { float get(); }
    // PPinfo
    property PostProcessInfo^ PPInfo { PostProcessInfo^ get(); }
    virtual	bool Process(float dt, PostProcessInfo^ PPInfo);
    void Create();
    PostProcessParamBase^ GetParam(PostProcessParamType param);
    void ResetParam(PostProcessParamType param);
    void Save(String^ name);
};

}
}
}
