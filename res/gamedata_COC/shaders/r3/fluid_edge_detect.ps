#include "fluid_common_render.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
// A full-screen edge detection pass to locate artifacts
//	these artifacts are located on a downsized version of the rayDataTexture
// We use a smaller texture both to accurately find all the depth artifacts 
//  when raycasting to this smaller size and to save on the cost of this pass
// Use col.a to find depth edges of objects occluding the smoke
// Use col.g to find the edges where the camera near plane cuts the smoke volume
//
float4 main(VS_OUTPUT_EDGE vIn) : SV_Target
{

    // We need eight samples (the centre has zero weight in both kernels).
    float4 col;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV00); 
    float g00 = col.a;
    if(col.g < 0)
        g00 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV01); 
    float g01 = col.a;
    if(col.g < 0)
        g01 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV02); 
    float g02 = col.a;
    if(col.g < 0)
        g02 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV10); 
    float g10 = col.a;
    if(col.g < 0)
        g10 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV12); 
    float g12 = col.a;
    if(col.g < 0)
        g12 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV20); 
    float g20 = col.a;
    if(col.g < 0)
        g20 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV21); 
    float g21 = col.a;
    if(col.g < 0)
        g21 *= -1;
    col = rayDataTexSmall.Sample(samPointClamp, vIn.textureUV22); 
    float g22 = col.a;
    if(col.g < 0)
        g22 *= -1;
    	
    // Sobel in horizontal dir.
    float sx = 0;
    sx -= g00;
    sx -= g01 * 2;
    sx -= g02;
    sx += g20;
    sx += g21 * 2;
    sx += g22;
    // Sobel in vertical dir - weights are just rotated 90 degrees.
    float sy = 0;
    sy -= g00;
    sy += g02;
    sy -= g10 * 2;
    sy += g12 * 2;
    sy -= g20;
    sy += g22;

    float e = EdgeDetectScalar(sx, sy, edgeThreshold);
    return float4(e,e,e,1);

}