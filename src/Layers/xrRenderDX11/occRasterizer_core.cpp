#include "stdafx.h"
#include "Layers/xrRender/occRasterizer.h"

namespace xray::render::fg
{
static occTri* currentTri = nullptr;
static u32 dwPixels = 0;
static float currentA[3], currentB[3], currentC[3];

const int BOTTOM = 0, TOP = 1;

static void i_order(float* A, float* B, float* C)
{
    float *min, *max, *mid;
    if (A[1] <= B[1])
    {
        if (B[1] <= C[1])
        {
            min = A;
            mid = B;
            max = C;
        }
        else
            if (A[1] <= C[1])
        {
            min = A;
            mid = C;
            max = B;
        }
        else
        {
            min = C;
            mid = A;
            max = B;
        }
    }
    else
    {
        if (A[1] <= C[1])
        {
            min = B;
            mid = A;
            max = C;
        }
        else
            if (B[1] <= C[1])
        {
            min = B;
            mid = C;
            max = A;
        }
        else
        {
            min = C;
            mid = B;
            max = A;
        }
    }

    currentA[0] = min[0] + 2;
    currentB[0] = mid[0] + 2;
    currentC[0] = max[0] + 2;
    currentA[1] = min[1] + 2;
    currentB[1] = mid[1] + 2;
    currentC[1] = max[1] + 2;
    currentA[2] = min[2];
    currentB[2] = mid[2];
    currentC[2] = max[2];
}

static void Vclamp(int& v, int a, int b)
{
    if (v < a)
        v = a;
    else if (v >= b)
        v = b - 1;
}

BOOL shared(occTri* T1, occTri* T2)
{
    if (T1 == T2)
        return TRUE;
    if (T1->adjacent[0] == T2)
        return TRUE;
    if (T1->adjacent[1] == T2)
        return TRUE;
    if (T1->adjacent[2] == T2)
        return TRUE;
    return FALSE;
}

const float one_div_3 = 1.f / 3.f;

void i_scan(int curY, float leftX, float lhx, float rightX, float rhx, float startZ, float endZ)
{
    float start_c = leftX + lhx;
    float end_c = rightX + rhx;

    float startR = leftX - lhx;
    float endR = rightX - rhx;

    float startT = startR, endT = end_c;
    float startX = start_c, endX = endR;
    if (start_c < startR)
    {
        startT = start_c;
        startX = startR;
    }
    if (end_c < endR)
    {
        endT = endR;
        endX = end_c;
    }

    int minT = iFloor(startT) - 1, maxT = iCeil(endT) + 1;
    Vclamp(minT, 1, occ_dim - 1);
    Vclamp(maxT, 1, occ_dim - 1);
    if (minT >= maxT)
        return;

    int minX = iCeil(startX), maxX = iFloor(endX);
    Vclamp(minX, 0, occ_dim);
    Vclamp(maxX, 0, occ_dim);
    int limLeft, limRight;
    if (minX > maxX)
    {
        limLeft = maxX;
        limRight = minX;
    }
    else
    {
        limLeft = minX;
        limRight = maxX;
    }

    float lenR = endR - startR;
    float Zlen = endZ - startZ;
    float Z = startZ + (minT - startR) / lenR * Zlen;
    float Zend = startZ + (maxT - startR) / lenR * Zlen;
    float dZ = (Zend - Z) / (maxT - minT);

    Z += 0.5f * _abs(dZ);

    occTri** pFrame = Raster.get_frame();
    float* pDepth = Raster.get_depth();

    int i_base = curY * occ_dim;
    int i = i_base + minT;
    int limit = i_base + limLeft;
    for (; i < limit; i++, Z += dZ)
    {
        if (shared(currentTri, pFrame[i - 1]))
        {
            if (Z < pDepth[i])
            {
                pFrame[i] = currentTri;
                pDepth[i] = __max(Z, pDepth[i - 1]);
                dwPixels++;
            }
        }
    }

    limit = i_base + maxX;
    for (; i < limit; i++, Z += dZ)
    {
        if (Z < pDepth[i])
        {
            pFrame[i] = currentTri;
            pDepth[i] = Z;
            dwPixels++;
        }
    }

    i = i_base + maxT - 1;
    limit = i_base + limRight;
    Z = Zend - dZ;
    for (; i >= limit; i--, Z -= dZ)
    {
        if (shared(currentTri, pFrame[i + 1]))
        {
            if (Z < pDepth[i])
            {
                pFrame[i] = currentTri;
                pDepth[i] = __max(Z, pDepth[i + 1]);
                dwPixels++;
            }
        }
    }
}

IC void i_test_micro(int x, int y)
{
    if (x < 1)
        return;
    else if (x >= occ_dim - 1)
        return;
    if (y < 1)
        return;
    else if (y >= occ_dim - 1)
        return;
    int pos = y * occ_dim + x;
    int pos_up = pos - occ_dim;
    int pos_down = pos + occ_dim;

    occTri** pFrame = Raster.get_frame();
    occTri* T1 = pFrame[pos_up];
    occTri* T2 = pFrame[pos_down];
    if (T1 && shared(T1, T2))
    {
        float* pDepth = Raster.get_depth();
        float ZR = (pDepth[pos_up] + pDepth[pos_down]) / 2;
        if (ZR < pDepth[pos])
        {
            pFrame[pos] = T1;
            pDepth[pos] = ZR;
        }
    }
}
void i_test(int x, int y)
{
    i_test_micro(x, y - 1);
    i_test_micro(x, y + 1);
    i_test_micro(x, y);
}

void i_edge(int x1, int y1, int x2, int y2)
{
    int dx = _abs(x2 - x1);
    int dy = _abs(y2 - y1);

    int sx = x2 >= x1 ? 1 : -1;
    int sy = y2 >= y1 ? 1 : -1;

    if (dy <= dx)
    {
        int d = (dy << 1) - dx;
        int d1 = dy << 1;
        int d2 = (dy - dx) << 1;

        i_test(x1, y1);
        for (int x = x1 + sx, y = y1, i = 1; i <= dx; i++, x += sx)
        {
            if (d > 0)
            {
                d += d2;
                y += sy;
            }
            else
                d += d1;
            i_test(x, y);
        }
    }
    else
    {
        int d = (dx << 1) - dy;
        int d1 = dx << 1;
        int d2 = (dx - dy) << 1;

        i_test(x1, y1);
        for (int x = x1, y = y1 + sy, i = 1; i <= dy; i++, y += sy)
        {
            if (d > 0)
            {
                d += d2;
                x += sx;
            }
            else
                d += d1;
            i_test(x, y);
        }
    }
}

IC void i_section(int Sect, BOOL bMiddle)
{
    int startY, endY;
    float *startp1, *startp2;
    float E1[3], E2[3];

    if (Sect == BOTTOM)
    {
        startY = iCeil(currentA[1]);
        endY = iFloor(currentB[1]) - 1;
        startp1 = startp2 = currentA;
        if (bMiddle)
            endY++;

        int test = iFloor(currentC[1]);
        if (endY >= test)
            endY--;

        E1[0] = currentB[0] - currentA[0];
        E2[0] = currentC[0] - currentA[0];
        E1[1] = currentB[1] - currentA[1];
        E2[1] = currentC[1] - currentA[1];
        E1[2] = currentB[2] - currentA[2];
        E2[2] = currentC[2] - currentA[2];
    }
    else
    {
        startY = iCeil(currentB[1]);
        endY = iFloor(currentC[1]);
        startp1 = currentA;
        startp2 = currentB;
        if (bMiddle)
            startY--;

        int test = iCeil(currentA[1]);
        if (startY < test)
            startY++;

        E1[0] = currentC[0] - currentA[0];
        E2[0] = currentC[0] - currentB[0];
        E1[1] = currentC[1] - currentA[1];
        E2[1] = currentC[1] - currentB[1];
        E1[2] = currentC[2] - currentA[2];
        E2[2] = currentC[2] - currentB[2];
    }
    Vclamp(startY, 0, occ_dim);
    Vclamp(endY, 0, occ_dim);
    if (startY >= endY)
        return;

    float mE1 = E1[0] / E1[1];
    float mE2 = E2[0] / E2[1];

    float e1_init_dY = float(startY) - startp1[1], e2_init_dY = float(startY) - startp2[1];
    float t, leftX, leftZ, rightX, rightZ, left_dX, right_dX, left_dZ, right_dZ;

    if (((mE1 < mE2) && (Sect == BOTTOM)) || ((mE1 > mE2) && (Sect == TOP)))
    {
        t = e1_init_dY / E1[1];
        leftX = startp1[0] + E1[0] * t;
        left_dX = mE1;
        leftZ = startp1[2] + E1[2] * t;
        left_dZ = E1[2] / E1[1];

        t = e2_init_dY / E2[1];
        rightX = startp2[0] + E2[0] * t;
        right_dX = mE2;
        rightZ = startp2[2] + E2[2] * t;
        right_dZ = E2[2] / E2[1];
    }
    else
    {
        t = e2_init_dY / E2[1];
        leftX = startp2[0] + E2[0] * t;
        left_dX = mE2;
        leftZ = startp2[2] + E2[2] * t;
        left_dZ = E2[2] / E2[1];

        t = e1_init_dY / E1[1];
        rightX = startp1[0] + E1[0] * t;
        right_dX = mE1;
        rightZ = startp1[2] + E1[2] * t;
        right_dZ = E1[2] / E1[1];
    }

    float lhx = left_dX / 2;
    leftX += lhx;
    float rhx = right_dX / 2;
    rightX += rhx;
    for (; startY <= endY; startY++)
    {
        i_scan(startY, leftX, lhx, rightX, rhx, leftZ, rightZ);
        leftX += left_dX;
        rightX += right_dX;
        leftZ += left_dZ;
        rightZ += right_dZ;
    }
}

void __stdcall i_section_b0() { i_section(BOTTOM, 0); }
void __stdcall i_section_b1() { i_section(BOTTOM, 1); }
void __stdcall i_section_t0() { i_section(TOP, 0); }
void __stdcall i_section_t1() { i_section(TOP, 1); }
u32 occRasterizer::rasterize(occTri* T)
{
    ZoneScoped;

    currentTri = T;
    dwPixels = 0;
    i_order(&(T->raster[0].x), &(T->raster[1].x), &(T->raster[2].x));

    if (currentB[1] - iFloor(currentB[1]) > .5f)
    {
        i_section_b1();
        i_section_t0();
    }
    else
    {
        i_section_b0();
        i_section_t1();
    }
    return dwPixels;
}
}
