float4 season_cb ;

float3 hsv2rgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float3 rgb2hsv(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
 
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 calc_season_rgb (float3 srccolor) 
{
	if (season_cb.x > 0.1)
	{
		float3 hsv = rgb2hsv(srccolor);
		hsv.g = clamp(pow(hsv.r - season_cb.x, 2.0) * season_cb.y, 0.0, hsv.g);
		hsv.b = lerp(hsv.b, hsv.b * 0.75, saturate(1.0/season_cb.y));
		
		if (season_cb.z > 0.1) 
		{
			hsv.g = clamp(pow(hsv.r - season_cb.z, 2.0) * season_cb.w, 0.0, hsv.g);
		}
		return hsv2rgb(hsv);
	} else
		return srccolor;
}