uniform	float4		screen_res;
#ifdef USE_MSAA
#ifndef USE_HBAO
Texture2DMS<float4, MSAA_SAMPLES>	s_occ;
#else
uniform	Texture2D					s_occ;
#endif
#else
uniform	Texture2D					s_occ;
#endif

#ifndef USE_MSAA
float ssao_blur_ps(float2 centerTC)
#else
float ssao_blur_ps(int2 centerTC, int iSample)
#endif
{
  // low weight center sample - will be used on edges
	float  fSumWeight = 0.025f;
#ifndef USE_MSAA
   float2 centerData = s_occ.Sample( smp_nofilter, centerTC);
#else
   float2 centerData = s_occ.Load( int3(centerTC, 0), iSample);
#endif

   float  fOcclusion  = centerData.r * fSumWeight;
   float  centerDepth = centerData.g;

#ifndef USE_MSAA
   float2 arrOffsets[4] =
   {
     float2( 1,-1),
     float2(-1,-1),
     float2( 1, 1),
     float2(-1, 1)
   };
#else
   int2 arrOffsets[4] =
   {
     int2( 1, -1),
     int2(-1, -1),
     int2( 1, 1),
     int2(-1, 1)
   };
#endif

  [unroll]
  for(int i=0; i<4; i++)
  {
#ifndef USE_MSAA
     float2 sampleTC   = centerTC + pos_decompression_params2.zw * arrOffsets[i];
     float2 sampleData = s_occ.Sample( smp_nofilter, sampleTC);
#else
     int2	sampleTC   = centerTC + arrOffsets[i];
     float2 sampleData = s_occ.Load(int3(sampleTC, 0), iSample);
#endif
	 
		float fDepth  = sampleData.g;
		float fDiff	  = 8*abs(fDepth-centerDepth)/min(fDepth,centerDepth);
     	float fWeight = saturate(1-fDiff);

     fOcclusion += sampleData.r * fWeight;

     fSumWeight += fWeight;
  }

   fOcclusion /= fSumWeight;   

  return fOcclusion;
}
// #else
// float ssao_blur_ps(int3 centertc, int isample)
// {
  // // low weight center sample - will be used on edges
   // float  fSumWeight = 10*0.0125f;
   // float2 centerData = s_occ.Load( centerTC, iSample);

   // float  cColor      = centerData.r * fSumWeight;
   // float  centerDepth = centerData.g;
	
   // int3 arrOffsets[4] =
   // {
     // int3( 1, -1, 0),
     // int3(-1, -1, 0),
     // int3( 1, 1, 0),
     // int3(-1, 1, 0)
   // };

  // for(int i=0; i<4; i++)
  // {
     // int3 sampleTC   = centerTC + arrOffsets[i];
     // float2 sampleData = s_occ.Load(sampleTC, iSample);
	 
     // float fDepth  = sampleData.g;
     // float fDiff   = 8.f*(1.f - fDepth/centerDepth);
     // float fWeight = saturate(0.5f - 0.75f*abs(fDiff) - 0.25f*(fDiff));

     // cColor += sampleData.r * fWeight;

     // fSumWeight += fWeight;
  // }

   // cColor /= fSumWeight;   

  // return cColor;
// }
// #endif
