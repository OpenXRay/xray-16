#pragma once

// refs
class XRSOUND_API CSoundRender_Environment : public CSound_environment
{
public:
    u32 version;
    shared_str name;

    u32 Environment; // source environment
    float Room; // room effect level at low frequencies
    float RoomHF; // room effect high-frequency level re. low frequency level
    float RoomRolloffFactor; // like DS3D flRolloffFactor but for room effect
    float DecayTime; // reverberation decay time at low frequencies
    float DecayHFRatio; // high-frequency to low-frequency decay time ratio
    float DecayLFRatio; // low-frequency to high-frequency decay time ratio
    int DecayHFLimit; // high- frequency decay time limit set by air absorbtion gain
    float Reflections; // early reflections level relative to room effect
    float ReflectionsDelay; // initial reflection delay time
    //Fvector3 ReflectionsPan; // reflections panning xyz
    float EchoTime; // delay between the original sound and echo instance
    float EchoDepth; // controls the amount of echo feedback signal in the loop
    //Fvector3 ReverbPan; // reverb pan xyz
    float Reverb; // late reverberation level relative to room effect
    float ReverbDelay; // late reverberation delay time relative to initial reflection
    float EnvironmentSize; // environment size in meters
    float EnvironmentDiffusion; // controls the echo density in the reverberation decay
    float AirAbsorptionHF; // change in level per meter at 5 kHz
    float ModulationTime; // modulation time for sound blending
    float ModulationDepth; // modulation depth for sound blending
    float Density; // controls the coloration of the late reverb
    float HFReference; // frequency at which low frequency effects are measured
    float LFReference; // frequency at which high frequency effects are measured

    CSoundRender_Environment();
    ~CSoundRender_Environment();
    void set_identity();
    void set_default();
    void clamp();
    void lerp(CSoundRender_Environment& A, CSoundRender_Environment& B, float f);
    bool load(IReader* fs);
    void save(IWriter* fs);
};

class XRSOUND_API SoundEnvironment_LIB
{
public:
    using SE_VEC = xr_vector<CSoundRender_Environment*>;

private:
    SE_VEC library;

public:
    void Load(pcstr name);
    bool Save(pcstr name);
    void Unload();
    int GetID(pcstr name);
    CSoundRender_Environment* Get(pcstr name);
    CSoundRender_Environment* Get(int id);
    CSoundRender_Environment* Append(CSoundRender_Environment* parent = nullptr);
    void Remove(pcstr name);
    void Remove(int id);
    SE_VEC& Library();
};
