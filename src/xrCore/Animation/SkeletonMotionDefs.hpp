#ifndef skeleton_motion_defs_inluded
#define skeleton_motion_defs_inluded

#pragma once

const u32 MAX_PARTS = 4;
const u32 MAX_BLENDED = 16;
const u32 MAX_CHANNELS = 4;
const u32 MAX_BLENDED_POOL = MAX_BLENDED * MAX_PARTS * MAX_CHANNELS;
const u32 MAX_ANIM_SLOT = 48;

const f32 SAMPLE_FPS = 30.f;
const f32 SAMPLE_SPF = (1.f / SAMPLE_FPS);
f32 const END_EPS = SAMPLE_SPF + EPS;
const f32 KEY_Quant = 32767.f;
const f32 KEY_QuantI = 1.f / KEY_Quant;

#endif
