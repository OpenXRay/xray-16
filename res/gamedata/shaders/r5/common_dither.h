#ifndef COMMON_DITHER_H
#define COMMON_DITHER_H

static const float kBayer4x4[16] =
{
    0.0, 8.0, 2.0, 10.0,
    12.0, 4.0, 14.0, 6.0,
    3.0, 11.0, 1.0, 9.0,
    15.0, 7.0, 13.0, 5.0
};

float Bayer4x4Threshold(uint2 position)
{
    return (kBayer4x4[(position.y & 3u) * 4u + (position.x & 3u)] + 0.5) / 16.0;
}

#endif
