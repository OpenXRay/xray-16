#pragma once

// normalize angle (0..2PI)
float angle_normalize_always(float a);

// normalize angle (0..2PI)
float angle_normalize(float a);

// -PI .. +PI
float angle_normalize_signed(float a);

// -PI..PI
float angle_difference_signed(float a, float b);

// 0..PI
float angle_difference(float a, float b);

bool are_ordered(const float value0, const float value1, const float value2);

bool is_between(const float value, const float left, const float right);

// c=current, t=target, s=speed, dt=dt
bool angle_lerp(float& c, float t, float s, float dt);

// Just lerp :) expects normalized angles in range [0..2PI)
float angle_lerp(float A, float B, float f);

float angle_inertion(float src, float tgt, float speed, float clmp, float dt);

float angle_inertion_var(float src, float tgt, float min_speed, float max_speed, float clmp, float dt);
