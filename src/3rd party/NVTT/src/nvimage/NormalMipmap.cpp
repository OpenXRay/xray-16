// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Ptr.h>

#include <nvmath/Montecarlo.h>
#include <nvmath/SphericalHarmonic.h>

#include <nvimage/NormalMipmap.h>
#include <nvimage/FloatImage.h>

using namespace nv;

FloatImage * nv::createNormalMipmapMap(const FloatImage * img)
{
	nvDebugCheck(img != NULL);
	
	uint w = img->width();
	uint h = img->height();
	
	uint hw = w / 2;
	uint hh = h / 2;
	
	FloatImage dotImg;
	dotImg.allocate(1, w, h);
	
	FloatImage shImg;
	shImg.allocate(9, hw, hh);
	
	SampleDistribution distribution(256);
	const uint sampleCount = distribution.sampleCount();
	
	for (uint d = 0; d < sampleCount; d++)
	{
		const float * xChannel = img->channel(0);
		const float * yChannel = img->channel(1);
		const float * zChannel = img->channel(2);
		
		Vector3 dir = distribution.sampleDir(d);
		
		Sh2 basis;
		basis.eval(dir);
		
		for(uint i = 0; i < w*h; i++)
		{
			Vector3 normal(xChannel[i], yChannel[i], zChannel[i]);
			normal = normalizeSafe(normal, Vector3(zero), 0.0f);
			
			dotImg.setPixel(dot(dir, normal), d);
		}
		
		// @@ It would be nice to have a fastDownSample that took an existing image as an argument, to avoid allocations.
		AutoPtr<FloatImage> dotMip(dotImg.fastDownSample());
		
		for(uint p = 0; p < hw*hh; p++)
		{
			float f = dotMip->pixel(p);
			
			// Project irradiance to sh basis and accumulate.
			for (uint i = 0; i < 9; i++)
			{
				float & sum = shImg.channel(i)[p];
				sum += f * basis.elemAt(i);
			}
		}
	}
	
	
	
	FloatImage * normalMipmap = new FloatImage;
	normalMipmap->allocate(4, hw, hh);
	
	// Precompute the clamped cosine radiance transfer.
	Sh2 prt;
	prt.cosineTransfer();
	
	// Allocate outside the loop.
	Sh2 sh;
	
	for(uint p = 0; p < hw*hh; p++)
	{
		for (uint i = 0; i < 9; i++)
		{
			sh.elemAt(i) = shImg.channel(i)[p];
		}
		
		// Convolve sh irradiance by radiance transfer.
		sh *= prt;
		
		// Now sh(0) is the ambient occlusion.
		// and sh(1) is the normal direction.
		
		// Should we use SVD to fit only the normals to the SH?
		
	}
	
	return normalMipmap;
}

