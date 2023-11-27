/////////////////////////////////
// BEGIN Custom Sampling Functions
Texture1D   HHGGTex;

// cubic b-spline
float bsW0(float a)
{
    return (1.0/6.0 * ( -(a*a*a) + (3.0 * a*a) - (3.0 * a) + 1.0));
}

float bsW1(float a)
{
    return (1.0/6.0 * ( (3.0 * a*a*a) - (6.0 * a*a) + 4.0 ));
}

float bsW2(float a)
{
    return (1.0/6.0 * ( -(3.0 * a*a*a) + (3.0 * a*a) + (3.0*a) + 1.0));
}

float bsW3(float a)
{
    return (1.0/6.0 * a*a*a);
}

float g0(float a)
{
    return (bsW0(a) + bsW1(a));
}

float g1(float a)
{
    return (bsW2(a) + bsW3(a));
}

float h0texels(float a)
{
    return (1.0 + a - (bsW1(a)/(bsW0(a)+bsW1(a))));
}

float h1texels(float a)
{
    return (1.0 - a + (bsW3(a)/(bsW2(a)+bsW3(a))));
}
/// end cubic-bspline

// first derivative of cubic b-spline
float bsfdW0(float a)
{
    return (1.0/6.0 * ( -(3.0 * a*a) + (6.0 * a) - 3.0));
}

float bsfdW1(float a)
{
    return (1.0/6.0 * ( (9.0 * a*a) - (12.0 * a) ));
}

float bsfdW2(float a)
{
    return (1.0/6.0 * ( -(9.0 * a*a) + (6.0 * a) + 3.0));
}

float bsfdW3(float a)
{
    return (1.0/6.0 * 3.0 * a*a);
}

float gfd0(float a)
{
    return (bsfdW0(a) + bsfdW1(a));
}

float gfd1(float a)
{
    return (bsfdW2(a) + bsfdW3(a));
}

float hfd0texels(float a)
{
    return (1.0 + a - (bsfdW1(a)/(bsfdW0(a)+bsfdW1(a))));
}

float hfd1texels(float a)
{
    return (1.0 - a + (bsfdW3(a)/(bsfdW2(a)+bsfdW3(a))));
}
/// end first derivative of cubic b-spline

float4 getHHGG( float xTexels)
{
//    float a = frac(xTexels);
//    return float4( -h0texels(a), h1texels(a), 1.0-g0(a), g0(a) );

	return HHGGTex.SampleLevel( samRepeat, xTexels, 0 );
}

float4 getfdHHGG( float xTexels)
{
    float a = frac(xTexels);
    return float4( -hfd0texels(a), hfd1texels(a), gfd1(a), -gfd1(a) );
}


float4 SampleTricubicGeneric(Texture3D tex, float3 tc, float4 hg_x, float4 hg_y, float4 hg_z)
{   
    float3  tc100, tc000, tc110, tc010,
            tc101, tc001, tc111, tc011;
    
    tc100 = tc;
    tc000 = tc;
    tc100.x += (hg_x.x * recGridDim.x);
    tc000.x += (hg_x.y * recGridDim.x);
    
    tc110 = tc100;
    tc010 = tc000;
    tc110.y += (hg_y.x * recGridDim.y);
    tc010.y += (hg_y.x * recGridDim.y);
    tc100.y += (hg_y.y * recGridDim.y);
    tc000.y += (hg_y.y * recGridDim.y);

    tc111 = tc110;
    tc011 = tc010;
    tc101 = tc100;
    tc001 = tc000;
    tc111.z += (hg_z.x * recGridDim.z);
    tc011.z += (hg_z.x * recGridDim.z);
    tc101.z += (hg_z.x * recGridDim.z);
    tc001.z += (hg_z.x * recGridDim.z);

    float4 v001 = tex.SampleLevel(samLinearClamp, tc001, 0);
    float4 v011 = tex.SampleLevel(samLinearClamp, tc011, 0);
    float4 v101 = tex.SampleLevel(samLinearClamp, tc101, 0);
    float4 v111 = tex.SampleLevel(samLinearClamp, tc111, 0);

    float4 v0Y1 = (v001 * hg_y.z) + (v011 * hg_y.w);
    float4 v1Y1 = (v101 * hg_y.z) + (v111 * hg_y.w);

    float4 vXY1 = (v0Y1 * hg_x.z) + (v1Y1 * hg_x.w);
    
    tc110.z += (hg_z.y * recGridDim.z);
    tc010.z += (hg_z.y * recGridDim.z);
    tc100.z += (hg_z.y * recGridDim.z);
    tc000.z += (hg_z.y * recGridDim.z);

    
    float4 v000 = tex.SampleLevel(samLinearClamp, tc000, 0);
    float4 v010 = tex.SampleLevel(samLinearClamp, tc010, 0);
    float4 v100 = tex.SampleLevel(samLinearClamp, tc100, 0);
    float4 v110 = tex.SampleLevel(samLinearClamp, tc110, 0);

    float4 v0Y0 = (v000 * hg_y.z) + (v010 * hg_y.w);
    float4 v1Y0 = (v100 * hg_y.z) + (v110 * hg_y.w);

    float4 vXY0 = (v0Y0 * hg_x.z) + (v1Y0 * hg_x.w);


    float4 vXYZ = (vXY0 * hg_z.z) + (vXY1 * hg_z.w);

    return vXYZ;
}


float4 SampleTricubic(Texture3D tex, float3 tc)
{
    float3 tcTexels = (tc * gridDim) - 0.49;

    float4 hg_x = getHHGG(tcTexels.x);
    float4 hg_y = getHHGG(tcTexels.y);
    float4 hg_z = getHHGG(tcTexels.z);

    return SampleTricubicGeneric(tex, tc, hg_x, hg_y, hg_z);
}

float4 SampleGradientTricubic(Texture3D tex, float3 tc)
{
    float3 tcTexels = (tc * gridDim) - 0.49;

    float4 hg_x   = getHHGG(tcTexels.x);
    float4 hg_y   = getHHGG(tcTexels.y);
    float4 hg_z   = getHHGG(tcTexels.z);
    float4 hgfd_x = getfdHHGG(tcTexels.x);
    float4 hgfd_y = getfdHHGG(tcTexels.y);
    float4 hgfd_z = getfdHHGG(tcTexels.z);

    return float4(  SampleTricubicGeneric(tex, tc, hgfd_x, hg_y, hg_z).r,
                    SampleTricubicGeneric(tex, tc, hg_x, hgfd_y, hg_z).r,
                    SampleTricubicGeneric(tex, tc, hg_x, hg_y, hgfd_z).r, 1.0 );
}


float4 SampleTrilinear(Texture3D tex, float3 tc)
{
    return tex.SampleLevel(samLinearClamp, tc, 0);
}

float4 SampleGradientTrilinear(Texture3D tex, float3 tc)
{
    #define LEFTCELL    float3 (tc.x-(1.0/gridDim.x), tc.y, tc.z)
    #define RIGHTCELL   float3 (tc.x+(1.0/gridDim.x), tc.y, tc.z)
    #define BOTTOMCELL  float3 (tc.x, (tc.y-(1.0/gridDim.y)), tc.z)
    #define TOPCELL     float3 (tc.x, (tc.y+(1.0/gridDim.y)), tc.z)
    #define DOWNCELL    float3 (tc.x, tc.y, tc.z - (1.0/gridDim.z))
    #define UPCELL      float3 (tc.x, tc.y, tc.z + (1.0/gridDim.z))

    float4 texL = tex.SampleLevel( samLinearClamp, LEFTCELL, 0 );
    float4 texR = tex.SampleLevel( samLinearClamp, RIGHTCELL, 0 );
    float4 texB = tex.SampleLevel( samLinearClamp, BOTTOMCELL, 0 );
    float4 texT = tex.SampleLevel( samLinearClamp, TOPCELL, 0 );
    float4 texU = tex.SampleLevel( samLinearClamp, UPCELL, 0 );
    float4 texD = tex.SampleLevel( samLinearClamp, DOWNCELL, 0 );
    return float4(  texR.r - texL.r, texT.r - texB.r, texU.r - texD.r, 1 );
}


float4 Sample(Texture3D tex, float3 tc)
{
    if( g_bRaycastFilterTricubic )
	{
        return SampleTricubic(tex, tc);
	}
    else
	{
        return SampleTrilinear(tex, tc);
	}
}

float4 SampleGradient(Texture3D tex, float3 tc)
{
    if( g_bRaycastFilterTricubic )
        return SampleGradientTricubic(tex, tc);
    else
        return SampleGradientTrilinear(tex, tc);
}
// END Custom Sampling Functions
/////////////////////////////////