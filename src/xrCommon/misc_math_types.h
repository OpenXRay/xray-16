#pragma once

struct SRotation
{
    float yaw, pitch, roll;

    SRotation() { yaw = pitch = roll = 0; }

    SRotation(float y, float p, float r)
    {
        yaw = y;
        pitch = p;
        roll = r;
    }
};
