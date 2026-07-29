#ifndef SHARED_TESS_PN_H
#define SHARED_TESS_PN_H

void TessComputePNPatch(
    float3 P0, float3 P1, float3 P2,
    float3 N0, float3 N1, float3 N2,
    out float3 b210, out float3 b120, out float3 b021,
    out float3 b012, out float3 b102, out float3 b201, out float3 b111,
    out float3 n110, out float3 n011, out float3 n101)
{
    b210 = (2.0 * P0 + P1 - dot(P1 - P0, N0) * N0) / 3.0;
    b120 = (2.0 * P1 + P0 - dot(P0 - P1, N1) * N1) / 3.0;
    b021 = (2.0 * P1 + P2 - dot(P2 - P1, N1) * N1) / 3.0;
    b012 = (2.0 * P2 + P1 - dot(P1 - P2, N2) * N2) / 3.0;
    b102 = (2.0 * P2 + P0 - dot(P0 - P2, N2) * N2) / 3.0;
    b201 = (2.0 * P0 + P2 - dot(P2 - P0, N0) * N0) / 3.0;

    float3 f3E = (b210 + b120 + b021 + b012 + b102 + b201) / 6.0;
    float3 f3V = (P0 + P1 + P2) / 3.0;
    b111 = f3E + ((f3E - f3V) / 2.0);

    float fV12 = 2.0 * dot(P1 - P0, N0 + N1) / max(dot(P1 - P0, P1 - P0), 1e-8);
    n110 = normalize(N0 + N1 - fV12 * (P1 - P0));
    float fV23 = 2.0 * dot(P2 - P1, N1 + N2) / max(dot(P2 - P1, P2 - P1), 1e-8);
    n011 = normalize(N1 + N2 - fV23 * (P2 - P1));
    float fV31 = 2.0 * dot(P0 - P2, N2 + N0) / max(dot(P0 - P2, P0 - P2), 1e-8);
    n101 = normalize(N2 + N0 - fV31 * (P0 - P2));
}

void TessPNEvaluate(
    float3 p0, float3 p1, float3 p2,
    float3 n0, float3 n1, float3 n2,
    float3 uvw,
    out float3 pos, out float3 N)
{
    float3 N0 = normalize(n0);
    float3 N1 = normalize(n1);
    float3 N2 = normalize(n2);

    float3 b210, b120, b021, b012, b102, b201, b111;
    float3 n110, n011, n101;
    TessComputePNPatch(p0, p1, p2, N0, N1, N2,
        b210, b120, b021, b012, b102, b201, b111,
        n110, n011, n101);

    float u = uvw.y;
    float v = uvw.x;
    float w = uvw.z;

    pos = p0 * w * w * w +
          p1 * u * u * u +
          p2 * v * v * v +
          b210 * 3.0 * w * w * u +
          b120 * 3.0 * w * u * u +
          b201 * 3.0 * w * w * v +
          b021 * 3.0 * u * u * v +
          b102 * 3.0 * w * v * v +
          b012 * 3.0 * u * v * v +
          b111 * 6.0 * w * u * v;

    N = n0 * w * w +
        n1 * u * u +
        n2 * v * v +
        n110 * w * u +
        n011 * u * v +
        n101 * w * v;
    N = normalize(N);
}

float TessHMDisplace(float height)
{
    return height * 0.07;
}

#endif
