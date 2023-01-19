#include "stdafx.h"

#include "SoundRender.h"
#include "SoundRender_Environment.h"

#define OPENAL
#include <openal/efx.h>

CSoundRender_Environment::CSoundRender_Environment()
{
    version = sdef_env_version;
    set_default();
}

CSoundRender_Environment::~CSoundRender_Environment() {}
void CSoundRender_Environment::set_default()
{
    Room = AL_EAXREVERB_DEFAULT_GAIN;
    RoomHF = AL_EAXREVERB_DEFAULT_GAINHF;
    RoomRolloffFactor = AL_EAXREVERB_DEFAULT_ROOM_ROLLOFF_FACTOR;
    DecayTime = AL_EAXREVERB_DEFAULT_DECAY_TIME;
    DecayHFRatio = AL_EAXREVERB_DEFAULT_DECAY_HFRATIO;
    DecayLFRatio = AL_EAXREVERB_DEFAULT_DECAY_LFRATIO;
    DecayHFLimit = AL_EAXREVERB_DEFAULT_DECAY_HFLIMIT;
    Reflections = AL_EAXREVERB_DEFAULT_REFLECTIONS_GAIN;
    ReflectionsDelay = AL_EAXREVERB_DEFAULT_REFLECTIONS_DELAY;
    ReflectionsPan = {0.f, 0.f, 0.f};
    EchoTime = AL_EAXREVERB_DEFAULT_ECHO_TIME;
    EchoDepth = AL_EAXREVERB_DEFAULT_ECHO_DEPTH;
    Reverb =  AL_EAXREVERB_DEFAULT_LATE_REVERB_GAIN;
    ReverbDelay = AL_EAXREVERB_DEFAULT_LATE_REVERB_DELAY;
    ReverbPan = {0.f, 0.f, 0.f};
    EnvironmentSize = AL_DEFAULT_METERS_PER_UNIT;
    EnvironmentDiffusion = AL_EAXREVERB_DEFAULT_DIFFUSION;
    AirAbsorptionHF = AL_EAXREVERB_DEFAULT_AIR_ABSORPTION_GAINHF;
    ModulationTime = AL_EAXREVERB_DEFAULT_MODULATION_TIME;
    ModulationDepth = AL_EAXREVERB_DEFAULT_MODULATION_DEPTH;
    Density = AL_EAXREVERB_DEFAULT_DENSITY;
    HFReference = AL_EAXREVERB_DEFAULT_HFREFERENCE;
    LFReference = AL_EAXREVERB_DEFAULT_LFREFERENCE;
}

void CSoundRender_Environment::set_identity()
{
    set_default();
    Room = AL_EAXREVERB_DEFAULT_GAIN;
    clamp();
}

void CSoundRender_Environment::lerp(CSoundRender_Environment& A, CSoundRender_Environment& B, float f)
{
    float fi = 1.f - f;

    Room = fi * A.Room + f * B.Room;
    RoomHF = fi * A.RoomHF + f * B.RoomHF;
    RoomRolloffFactor = fi * A.RoomRolloffFactor + f * B.RoomRolloffFactor;
    DecayTime = fi * A.DecayTime + f * B.DecayTime;
    DecayHFRatio = fi * A.DecayHFRatio + f * B.DecayHFRatio;
    DecayLFRatio = fi * A.DecayLFRatio + f * B.DecayLFRatio;
    Reflections = fi * A.Reflections + f * B.Reflections;
    ReflectionsDelay = fi * A.ReflectionsDelay + f * B.ReflectionsDelay;
    EchoTime = fi * A.EchoTime + f * B.EchoTime;
    EchoDepth = fi * A.EchoDepth + f * B.EchoDepth;
    Reverb = fi * A.Reverb + f * B.Reverb;
    ReverbDelay = fi * A.ReverbDelay + f * B.ReverbDelay;
    EnvironmentSize = fi * A.EnvironmentSize + f * B.EnvironmentSize;
    EnvironmentDiffusion = fi * A.EnvironmentDiffusion + f * B.EnvironmentDiffusion;
    AirAbsorptionHF = fi * A.AirAbsorptionHF + f * B.AirAbsorptionHF;
    ModulationTime = fi * A.ModulationTime + f * B.ModulationTime;
    ModulationDepth = fi * A.ModulationDepth + f * B.ModulationDepth;
    Density = fi * A.Density + f * B.Density;
    HFReference = fi * A.HFReference + f * B.HFReference;
    LFReference = fi * A.LFReference + f * B.LFReference;

    clamp();
}

/*
void CSoundRender_Environment::get			(EAXLISTENERPROPERTIES& ep)
{
    ep.lRoom					= iFloor(Room)					;	// room effect level at low frequencies
    ep.lRoomHF					= iFloor(RoomHF)				;   // room effect high-frequency level re. low
frequency level
    ep.flRoomRolloffFactor		= RoomRolloffFactor				;   // like DS3D flRolloffFactor but for room effect
    ep.flDecayTime				= DecayTime						;   // reverberation decay time at low frequencies
    ep.flDecayHFRatio			= DecayHFRatio					;   // high-frequency to low-frequency decay time ratio
    ep.lReflections				= iFloor(Reflections)			;   // early reflections level relative to room effect
    ep.flReflectionsDelay		= ReflectionsDelay				;   // initial reflection delay time
    ep.lReverb					= iFloor(Reverb)	 			;   // late reverberation level relative to room effect
    ep.flReverbDelay			= ReverbDelay					;   // late reverberation delay time relative to
initial reflection
    ep.dwEnvironment			= EAXLISTENER_DEFAULTENVIRONMENT;  alEffectfv	// sets all listener properties
    ep.flEnvironmentSize		= EnvironmentSize				;  	// environment size in meters
    ep.flEnvironmentDiffusion	= EnvironmentDiffusion			; 	// environment diffusion
    ep.flAirAbsorptionHF		= AirAbsorptionHF				;	// change in level per meter at 5 kHz
    ep.dwFlags					= EAXLISTENER_DEFAULTFLAGS		;	// modifies the behavior of properties
}
*/
void CSoundRender_Environment::clamp()
{
    ::clamp(Room, (float)AL_EAXREVERB_MIN_GAIN, (float)AL_EAXREVERB_MAX_GAIN);
    ::clamp(RoomHF, (float)AL_EAXREVERB_MIN_GAINHF, (float)AL_EAXREVERB_MAX_GAINHF);
    ::clamp(RoomRolloffFactor,AL_EAXREVERB_MIN_ROOM_ROLLOFF_FACTOR, AL_EAXREVERB_MAX_ROOM_ROLLOFF_FACTOR);
    ::clamp(DecayTime, AL_EAXREVERB_MIN_DECAY_TIME, AL_EAXREVERB_MAX_DECAY_TIME);
    ::clamp(DecayHFRatio, AL_EAXREVERB_MIN_DECAY_HFRATIO, AL_EAXREVERB_MAX_DECAY_HFRATIO);
    ::clamp(DecayLFRatio, AL_EAXREVERB_MIN_DECAY_LFRATIO, AL_EAXREVERB_MAX_DECAY_LFRATIO);
    ::clamp(Reflections, (float)AL_EAXREVERB_MIN_REFLECTIONS_GAIN, (float)AL_EAXREVERB_MAX_REFLECTIONS_GAIN);
    ::clamp(ReflectionsDelay, AL_EAXREVERB_MIN_REFLECTIONS_DELAY, (float)AL_EAXREVERB_MAX_REFLECTIONS_DELAY);
    ::clamp(EchoTime, AL_EAXREVERB_MIN_ECHO_TIME, AL_EAXREVERB_MAX_ECHO_TIME);
    ::clamp(EchoTime, AL_EAXREVERB_MIN_ECHO_DEPTH, AL_EAXREVERB_MAX_ECHO_DEPTH);
    ::clamp(Reverb, (float)AL_EAXREVERB_MIN_LATE_REVERB_GAIN, (float)AL_EAXREVERB_MAX_LATE_REVERB_GAIN);
    ::clamp(ReverbDelay, AL_EAXREVERB_MIN_LATE_REVERB_DELAY, AL_EAXREVERB_MAX_LATE_REVERB_DELAY);
    ::clamp(EnvironmentDiffusion, AL_EAXREVERB_MIN_DIFFUSION, AL_EAXREVERB_MAX_DIFFUSION);
    ::clamp(AirAbsorptionHF, AL_EAXREVERB_MIN_AIR_ABSORPTION_GAINHF, AL_EAXREVERB_MAX_AIR_ABSORPTION_GAINHF);
    ::clamp(ModulationTime, AL_EAXREVERB_MIN_MODULATION_TIME, AL_EAXREVERB_MAX_MODULATION_TIME);
    ::clamp(ModulationDepth, AL_EAXREVERB_MIN_MODULATION_DEPTH, AL_EAXREVERB_MAX_MODULATION_DEPTH);
    ::clamp(Density, AL_EAXREVERB_MIN_DENSITY, AL_EAXREVERB_MAX_DENSITY);
    ::clamp(HFReference, AL_EAXREVERB_MIN_HFREFERENCE, AL_EAXREVERB_MAX_HFREFERENCE);
    ::clamp(LFReference, AL_EAXREVERB_MIN_LFREFERENCE, AL_EAXREVERB_MAX_LFREFERENCE);
    ::clamp(DecayHFLimit, AL_EAXREVERB_MIN_DECAY_HFLIMIT, AL_EAXREVERB_MAX_DECAY_HFLIMIT);
}

bool CSoundRender_Environment::load(IReader* fs)
{
    version = fs->r_u32();

    if (version >= 0x0003)
    {
        fs->r_stringZ(name);

        Room = fs->r_float();
        RoomHF = fs->r_float();
        RoomRolloffFactor = fs->r_float();
        DecayTime = fs->r_float();
        DecayHFRatio = fs->r_float();
        DecayLFRatio = fs->r_float();
        DecayHFLimit = fs->r_u32();
        Reflections = fs->r_float();
        ReflectionsDelay = fs->r_float();
        ReflectionsPan = fs->r_vec3();
        EchoTime = fs->r_float();
        EchoDepth = fs->r_float();
        Reverb = fs->r_float();
        ReverbDelay = fs->r_float();
        ReverbPan = fs->r_vec3();
        EnvironmentSize = fs->r_float();
        EnvironmentDiffusion = fs->r_float();
        AirAbsorptionHF = fs->r_float();
        ModulationTime = fs->r_float();
        ModulationDepth = fs->r_float();
        HFReference = fs->r_float();
        LFReference = fs->r_float();
        Density = fs->r_float();
        return true;
    }
    return false;
}

void CSoundRender_Environment::save(IWriter* fs)
{
    fs->w_u32(sdef_env_version);
    fs->w_stringZ(name);

    fs->w_float(Room);
    fs->w_float(RoomHF);
    fs->w_float(RoomRolloffFactor);
    fs->w_float(DecayTime);
    fs->w_float(DecayHFRatio);
    fs->w_float(DecayLFRatio);
    fs->w_float(Reflections);
    fs->w_float(ReflectionsDelay);
    fs->w_fvector3(ReflectionsPan);
    fs->w_float(EchoTime);
    fs->w_float(EchoDepth);
    fs->w_float(Reverb);
    fs->w_float(ReverbDelay);
    fs->w_fvector3(ReverbPan);
    fs->w_float(EnvironmentSize);
    fs->w_float(EnvironmentDiffusion);
    fs->w_float(AirAbsorptionHF);
    fs->w_float(ModulationTime);
    fs->w_float(ModulationDepth);
    fs->w_float(Density);
    fs->w_float(HFReference);
    fs->w_float(LFReference);
    fs->w_u32(DecayHFLimit);
}

//////////////////////////////////////////////////////////////////////////
void SoundEnvironment_LIB::Load(pcstr name)
{
    R_ASSERT(library.empty());
    IReader* F = FS.r_open(name);
    IReader* C;
    library.reserve(256);
    for (u32 chunk = 0; nullptr != (C = F->open_chunk(chunk)); chunk++)
    {
        CSoundRender_Environment* E = xr_new<CSoundRender_Environment>();
        if (E->load(C))
            library.push_back(E);
        C->close();
    }
    FS.r_close(F);
}
bool SoundEnvironment_LIB::Save(pcstr name)
{
    IWriter* F = FS.w_open(name);
    if (F)
    {
        for (u32 chunk = 0; chunk < library.size(); chunk++)
        {
            F->open_chunk(chunk);
            library[chunk]->save(F);
            F->close_chunk();
        }
        FS.w_close(F);
        return true;
    }
    return false;
}
void SoundEnvironment_LIB::Unload()
{
    for (auto& lib : library)
        xr_delete(lib);
    library.clear();
}
int SoundEnvironment_LIB::GetID(pcstr name)
{
    for (auto it = library.begin(); it != library.end(); ++it)
        if (0 == xr_stricmp(name, *(*it)->name))
            return int(it - library.begin());
    return -1;
}
CSoundRender_Environment* SoundEnvironment_LIB::Get(pcstr name)
{
    for (const auto& it : library)
        if (0 == xr_stricmp(name, *it->name))
            return it;
    return nullptr;
}
CSoundRender_Environment* SoundEnvironment_LIB::Get(int id) { return library[id]; }
CSoundRender_Environment* SoundEnvironment_LIB::Append(CSoundRender_Environment* parent)
{
    library.push_back(parent ? xr_new<CSoundRender_Environment>(*parent) : xr_new<CSoundRender_Environment>());
    return library.back();
}
void SoundEnvironment_LIB::Remove(pcstr name)
{
    for (auto it = library.begin(); it != library.end(); ++it)
        if (0 == xr_stricmp(name, *(*it)->name))
        {
            xr_delete(*it);
            library.erase(it);
            break;
        }
}
void SoundEnvironment_LIB::Remove(int id)
{
    xr_delete(library[id]);
    library.erase(library.begin() + id);
}
SoundEnvironment_LIB::SE_VEC& SoundEnvironment_LIB::Library() { return library; }
