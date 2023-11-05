//=================================================================================================
// LMT for fixing occasional image artifacts seen in bright highlights (e.g. sirens,headlights,etc.)
// Note that this will change scene colorimetry! (but tends to do so in a pleasing way)
//=================================================================================================
static const float3x3 correctionMatrix =
{
	{ 0.9404372683,  0.0083786969,  0.0005471261},
	{-0.0183068787,  0.8286599939, -0.0008833746},
	{ 0.0778696104,  0.1629613092,  1.0003362486}
};

void Blue_Fix( inout float3 aces)
{
	aces = mul(aces, correctionMatrix );
}
