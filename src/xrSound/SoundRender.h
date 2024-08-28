#pragma once

#include "Sound.h"

class CSoundRender_Core;
class CSoundRender_Source;
class CSoundRender_Emitter;
class CSoundRender_EmitterParams;
class CSoundRender_Target;
class CSoundRender_Environment;
class SoundEnvironment_LIB;

const u32 sdef_target_count_submit = 4; // amount of buffers should be submitted to API
const u32 sdef_target_count_prefill = 10; //
const u32 sdef_target_block = 100; // ms
const u32 sdef_env_version = 4; // current version of env-def
const u32 sdef_level_version = 1; // current version of level-def
const float s_f_def_event_pulse = 0.5f; // sec
