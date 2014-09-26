// This code is in the public domain -- castanyo@yahoo.es

#include <nvcore/Containers.h>
#include <nvcore/Ptr.h>

#include <nvmath/nvmath.h>

#include <nvimage/HoleFilling.h>
#include <nvimage/FloatImage.h>

using namespace nv;


// This is a variation of Sapiro's inpainting method.
void nv::fillExtrapolate(int passCount, FloatImage * img, BitMap * bmap)
{
	nvCheck(img != NULL);
	nvCheck(bmap != NULL);

	const int w = img->width();
	const int h = img->height();
	const int count = img->componentNum();

	nvCheck(bmap->width() == uint(w));
	nvCheck(bmap->height() == uint(h));

	AutoPtr<BitMap> newbmap(new BitMap(w, h));

	for(int p = 0; p < passCount; p++)
	{
		for(int c = 0; c < count; c++)
		{
			float * channel = img->channel(c);
			
			for(int y = 0; y < h; y++) {
				for(int x = 0; x < w; x++) {
					
					if (bmap->bitAt(x, y)) {
						// Not a hole.
						newbmap->setBitAt(x, y);
						continue;
					}
					
					const bool west = bmap->bitAt(img->indexClamp(x-1, y));
					const bool east = bmap->bitAt(img->indexClamp(x+1, y));
					const bool north = bmap->bitAt(img->indexClamp(x, y-1));
					const bool south = bmap->bitAt(img->indexClamp(x, y+1));
					const bool northwest = bmap->bitAt(img->indexClamp(x-1, y-1));
					const bool northeast = bmap->bitAt(img->indexClamp(x+1, y-1));
					const bool southwest = bmap->bitAt(img->indexClamp(x-1, y+1));
					const bool southeast = bmap->bitAt(img->indexClamp(x+1, y+1));
					
					int num = west + east + north + south + northwest + northeast + southwest + southeast;
					
					if (num != 0) {

						float average = 0.0f;
						if (num == 3 && west && northwest && southwest) {
							average = channel[img->indexClamp(x-1, y)];
						}
						else if (num == 3 && east && northeast && southeast) {
							average = channel[img->indexClamp(x+1, y)];
						}
						else if (num == 3 && north && northwest && northeast) {
							average = channel[img->indexClamp(x, y-1)];
						}
						else if (num == 3 && south && southwest && southeast) {
							average = channel[img->indexClamp(x, y+1)];
						}
						else {
							float total = 0.0f;
							if (west) { average += 1 * channel[img->indexClamp(x-1, y)]; total += 1; }
							if (east) { average += 1 * channel[img->indexClamp(x+1, y)]; total += 1; }
							if (north) { average += 1 * channel[img->indexClamp(x, y-1)]; total += 1; }
							if (south) { average += 1 * channel[img->indexClamp(x, y+1)]; total += 1; }
						
							if (northwest) { average += channel[img->indexClamp(x-1, y-1)]; ++total; }
							if (northeast) { average += channel[img->indexClamp(x+1, y-1)]; ++total; }
							if (southwest) { average += channel[img->indexClamp(x-1, y+1)]; ++total; }
							if (southeast) { average += channel[img->indexClamp(x+1, y+1)]; ++total; }
							
							average /= total;
						}

						channel[img->indexClamp(x, y)] = average;
						newbmap->setBitAt(x, y);
					}
				}
			}
		}

		// Update the bit mask.
		swap(*newbmap, *bmap);
	}
}


namespace {

	struct Neighbor {
		uint16 x;
		uint16 y;
		uint32 d;
	};

	// Compute euclidean squared distance.
	static uint dist( uint16 ax, uint16 ay, uint16 bx, uint16 by ) {
		int dx = bx - ax;
		int dy = by - ay;
		return uint(dx*dx + dy*dy);
	}
	
	// Check neighbour, this is the core of the EDT algorithm.
	static void checkNeighbour( int x, int y, Neighbor * e, const Neighbor & n ) {
		nvDebugCheck(e != NULL);
		
		uint d = dist( x, y, n.x, n.y );
		if( d < e->d ) {
			e->x = n.x;
			e->y = n.y;
			e->d = d;
		}
	}

} // namespace

// Voronoi filling using EDT-4
void nv::fillVoronoi(FloatImage * img, const BitMap * bmap)
{
	nvCheck(img != NULL);

	const int w = img->width();
	const int h = img->height();
	const int count = img->componentNum();

	nvCheck(bmap->width() == uint(w));
	nvCheck(bmap->height() == uint(h));

	Array<Neighbor> edm;
	edm.resize(w * h);
	
	int x, y;
	int x0, x1, y0, y1;

	// Init edm.
	for( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ ) {
			if( bmap->bitAt(x, y) ) {
				edm[y * w + x].x = x;
				edm[y * w + x].y = y;
				edm[y * w + x].d = 0;
			}
			else {
				edm[y * w + x].x = w;
				edm[y * w + x].y = h;
				edm[y * w + x].d = w*w + h*h;
			}
		}
	}
	
	// First pass.
	for( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ ) {
			x0 = clamp(x-1, 0, w-1);	// @@ Wrap?
			x1 = clamp(x+1, 0, w-1);
			y0 = clamp(y-1, 0, h-1);
			
			Neighbor & e = edm[y * w + x];
			checkNeighbour(x, y, &e, edm[y0 * w + x0]);
			checkNeighbour(x, y, &e, edm[y0 * w + x]);
			checkNeighbour(x, y, &e, edm[y0 * w + x1]);
			checkNeighbour(x, y, &e, edm[y * w + x0]);
		}
		
		for( x = w-1; x >= 0; x-- ) {
			x1 = clamp(x+1, 0, w-1);
			
			Neighbor & e = edm[y * w + x];
			checkNeighbour(x, y, &e, edm[y * w + x1]);
		}
	}
	
	// Third pass.
	for( y = h-1; y >= 0; y-- ) {
		for( x = w-1; x >= 0; x-- ) {
			x0 = clamp(x-1, 0, w-1);
			x1 = clamp(x+1, 0, w-1);
			y1 = clamp(y+1, 0, h-1);
			
			Neighbor & e = edm[y * w + x];
			checkNeighbour(x, y, &e, edm[y * w + x1]);
			checkNeighbour(x, y, &e, edm[y1 * w + x0]);
			checkNeighbour(x, y, &e, edm[y1 * w + x]);
			checkNeighbour(x, y, &e, edm[y1 * w + x1]);
		}
		
		for( x = 0; x < w; x++ ) {
			x0 = clamp(x-1, 0, w-1);
			
			Neighbor & e = edm[y * w + x];
			checkNeighbour(x, y, &e, edm[y * w + x0]);
		}
	}
	
	// Fill empty holes.
	for( y = 0; y < h; y++ ) {
		for( x = 0; x < w; x++ ) {
			const int sx = edm[y * w + x].x;
			const int sy = edm[y * w + x].y;
			nvDebugCheck(sx < w && sy < h);
			
			if( sx != x || sy != y ) {
				for(int c = 0; c < count; c++ ) {
					img->setPixel(img->pixel(sx, sy, c), x, y, c);
				}
			}
		}
	}

}


void nv::fillBlur(FloatImage * img, const BitMap * bmap)
{
	nvCheck(img != NULL);
	
	// @@ Apply a 3x3 kernel.
}


static bool downsample(const FloatImage * src, const BitMap * srcMask, const FloatImage ** _dst, const BitMap ** _dstMask)
{
	const uint w = src->width();
	const uint h = src->height();
	const uint count = src->componentNum();

	// count holes in srcMask, return false if fully filled.
	uint holes = 0;
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {
			holes += srcMask->bitAt(x, y) == 0;
		}
	}
	if (holes == 0 || (w == 2 || h == 2)) {
		// Stop when no holes or when the texture is very small.
		return false;
	}

	// Apply box filter to image and mask and return true.
	const uint nw = w / 2;
	const uint nh = h / 2;

	FloatImage * dst = new FloatImage();
	dst->allocate(count, nw, nh);
	BitMap * dstMask = new BitMap(nw, nh);

	for(uint c = 0; c < count; c++) {
		for(uint y = 0; y < nh; y++) {
			for(uint x = 0; x < nw; x++) {

				const uint x0 = 2 * x + 0;
				const uint x1 = 2 * x + 1;
				const uint y0 = 2 * y + 0;
				const uint y1 = 2 * y + 1;

				const float f0 = src->pixel(x0, y0, c);
				const float f1 = src->pixel(x1, y0, c);
				const float f2 = src->pixel(x0, y1, c);
				const float f3 = src->pixel(x1, y1, c);

				const bool b0 = srcMask->bitAt(x0, y0);
				const bool b1 = srcMask->bitAt(x1, y0);
				const bool b2 = srcMask->bitAt(x0, y1);
				const bool b3 = srcMask->bitAt(x1, y1);

				if (b0 || b1 || b2 || b3) {
					// Set bit mask.
					dstMask->setBitAt(x, y);

					// Set pixel.
					float value = 0.0f;
					int total = 0;
					if (b0) { value += f0; total++; }
					if (b1) { value += f1; total++; }
					if (b2) { value += f2; total++; }
					if (b3) { value += f3; total++; }
					dst->setPixel(value / total, x, y, c);
				}
			}
		}
	}

	*_dst = dst;
	*_dstMask = dstMask;

	return true;
}

// This is the filter used in the Lumigraph paper.
void nv::fillPullPush(FloatImage * img, const BitMap * bmap)
{
	nvCheck(img != NULL);

	const uint count = img->componentNum();
	const uint w = img->width();
	const uint h = img->height();
	const uint num = log2(max(w,h));

	// Build mipmap chain.
	Array<const FloatImage *> mipmaps(num);
	Array<const BitMap *> mipmapMasks(num);

	mipmaps.append(img);
	mipmapMasks.append(bmap);

	const FloatImage * current;
	const BitMap * currentMask;

	// Compute mipmap chain.
	while(downsample(mipmaps.back(), mipmapMasks.back(), &current, &currentMask))
	{
		mipmaps.append(current);
		mipmapMasks.append(currentMask);
	}

	// Sample mipmaps until non-hole is found.
	for(uint y = 0; y < h; y++) {
		for(uint x = 0; x < w; x++) {

			int sx = x;
			int sy = y;
			//float sx = x;
			//float sy = y;

			const uint levelCount = mipmaps.count();
			for (uint l = 0; l < levelCount; l++)
			{
				//const float fx = sx / mipmaps[l]->width();
				//const float fy = sy / mipmaps[l]->height();

				if (mipmapMasks[l]->bitAt(sx, sy))
				{
					// Sample mipmaps[l](sx, sy) and copy to img(x, y)
					for(uint c = 0; c < count; c++) {
						//img->setPixel(mipmaps[l]->linear_clamp(fx, fy, c), x, y, c);
						img->setPixel(mipmaps[l]->pixel(sx, sy, c), x, y, c);
					}
					break;
				}

				sx /= 2;
				sy /= 2;
			}
		}
	}

	// Don't delete the original image and mask.
	mipmaps[0] = NULL;
	mipmapMasks[0] = NULL;

	// Delete the mipmaps.
	deleteAll(mipmaps);
	deleteAll(mipmapMasks);
}



/*

This Code is from Charles Bloom:

DoPixelSeamFix
10-20-02

Looks in the 5x5 local neighborhood (LocalPixels) of the desired pixel to fill.
It tries to build a quadratic model of the neighborhood surface to use in
extrapolating.  You need 5 pixels to establish a 2d quadratic curve.

This is really just a nice generic way to extrapolate pixels.  It also happens
to work great for seam-fixing.

Note that I'm working on normals, but I treat them just as 3 scalars and normalize
at the end.  To be more correct, I would work on the surface of a sphere, but that
just seems like way too much work.

*/

struct LocalPixels
{
	// 5x5 neighborhood
	// the center is at result
	// index [y][x]
	bool fill[5][5];
	float data[5][5];
	
	mutable float result;
	mutable float weight;

	bool Quad3SubH(float * pQ, int row) const
	{
		const bool * pFill = fill[row];
		const float * pDat = data[row];
	
		if ( pFill[1] && pFill[2] && pFill[3] )
		{
			// good row
			*pQ = pDat[1] - 2.f * pDat[2] + pDat[3];
			return true;
		}
		else if ( pFill[0] && pFill[1] && pFill[2] )
		{
			// good row
			*pQ = pDat[0] - 2.f * pDat[1] + pDat[2];
			return true;
		}
		else if ( pFill[2] && pFill[3] && pFill[4] )
		{
			// good row
			*pQ = pDat[2] - 2.f * pDat[3] + pDat[4];
			return true;
		}
		return false;
	}

	// improve result with a horizontal quad in row 1 and/or 
	bool Quad3SubV(float * pQ, int col) const
	{
		if ( fill[1][col] && fill[2][col] && fill[3][col] )
		{
			// good row
			*pQ = data[1][col] - 2.f * data[2][col] + data[3][col];
			return true;
		}
		else if ( fill[0][col] && fill[1][col] && fill[2][col] )
		{
			// good row
			*pQ = data[0][col] - 2.f * data[1][col] + data[2][col];
			return true;
		}
		else if ( fill[2][col] && fill[3][col] && fill[4][col] )
		{
			// good row
			*pQ = data[2][col] - 2.f * data[3][col] + data[4][col];
			return true;
		}
		return false;
	}
	
	bool Quad3H(float * pQ) const
	{
		if (!Quad3SubH(pQ,1))
		{
			return Quad3SubH(pQ,3);	
		}
		float q = 0.0f; // initializer not needed, just make it shut up
		if (Quad3SubH(&q, 3))
		{
			// got q and pQ
			*pQ = (*pQ+q)*0.5f;
		}
		return true;
	}
	
	bool Quad3V(float * pQ) const
	{
		if (!Quad3SubV(pQ, 1))
		{
			return Quad3SubV(pQ, 3);	
		}
		float q = 0.0f; // initializer not needed, just make it shut up
		if (Quad3SubV(&q, 3))
		{
			// got q and pQ
			*pQ = (*pQ + q) * 0.5f;
		}
		return true;
	}
	// Quad returns ([0]+[2] - 2.f*[1])
	//	a common want is [1] - ([0]+[2])*0.5f ;
	// so use -0.5f*Quad

	bool tryQuads() const
	{
		bool res = false;
	
		// look for a pair that straddles the middle:
		if ( fill[2][1] && fill[2][3] )
		{
			// got horizontal straddle
			float q;
			if ( Quad3H(&q) )
			{
				result += (data[2][1] + data[2][3] - q) * 0.5f;
				weight += 1.f;
				res = true;
			}
		}
		if ( fill[1][2] && fill[3][2] )
		{
			// got vertical straddle
			float q;
			if ( Quad3V(&q) )
			{
				result += (data[1][2] + data[3][2] - q) * 0.5f;
				weight += 1.f;
				res = true;
			}
		}
	
		// look for pairs that lead into the middle :
		if ( fill[2][0] && fill[2][1] )
		{
			// got left-side pair
			float q;
			if ( Quad3H(&q) )
			{
				result += data[2][1]*2.f - data[2][0] + q;
				weight += 1.f;
				res = true;
			}
		}
		if ( fill[2][3] && fill[2][4] )
		{
			// got right-side pair
			float q;
			if ( Quad3H(&q) )
			{
				result += data[2][3]*2.f - data[2][4] + q;
				weight += 1.f;
				res = true;
			}
		}
		if ( fill[0][2] && fill[1][2] )
		{
			// got left-side pair
			float q;
			if ( Quad3V(&q) )
			{
				result += data[1][2]*2.f - data[0][2] + q;
				weight += 1.f;
				res = true;
			}
		}
		if ( fill[3][2] && fill[4][2] )
		{
			// got right-side pair
			float q;
			if ( Quad3V(&q) )
			{
				result += data[3][2]*2.f - data[4][2] + q;
				weight += 1.f;
				res = true;
			}
		}
		return res;
	}
	
	bool tryPlanar() const
	{
		// four cases :
		const int indices[] =
		{
			2,1, 1,2, 1,1,
			2,1, 3,2, 3,1,
			2,3, 1,2, 1,3,
			2,3, 3,2, 3,3
		};
		bool res = false;
		for (int i = 0; i < 4; i++)
		{
			const int * I = indices + i*6;
			if (!fill[ I[0] ][ I[1] ])
				continue;
			if (!fill[ I[2] ][ I[3] ])
				continue;
			if (!fill[ I[4] ][ I[5] ])
				continue;
	
			result += data[ I[0] ][ I[1] ] + data[ I[2] ][ I[3] ] - data[ I[4] ][ I[5] ];
			weight += 1.0f;
			res = true;
		}
		return res;
	}
	
	bool tryTwos() const
	{
		bool res = false;
	
		if (fill[2][1] && fill[2][3])
		{
			result += (data[2][1] + data[2][3]) * 0.5f;
			weight += 1.0f;
			res = true;
		}
		if (fill[1][2] && fill[3][2])
		{
			result += (data[1][2] + data[3][2]) * 0.5f;
			weight += 1.0f;
			res = true;
		}
		
		// four side-rotates :
		const int indices[] =
		{
			2,1, 2,0,
			2,3, 2,4,
			1,2, 0,2,
			3,2, 4,2,
		};
		for (int i = 0; i < 4; i++)
		{
			const int * I = indices + i*4;
			if (!fill[ I[0] ][ I[1] ])
				continue;
			if (!fill[ I[2] ][ I[3] ])
				continue;
	
			result += data[ I[0] ][ I[1] ]*2.0f - data[ I[2] ][ I[3] ];
			weight += 1.0f;
			res = true;
		}
	
		return res;
	}

	bool doLocalPixelFill() const
	{
		result = 0.0f;
		weight = 0.0f;
		
		if (tryQuads()) {
			return true;
		}
		
		if (tryPlanar()) {
			return true;
		}
		
		return tryTwos();
	}

}; // struct LocalPixels



// This is a quadratic extrapolation filter from Charles Bloom (DoPixelSeamFix). Used with his permission.
void nv::fillQuadraticExtrapolate(int passCount, FloatImage * img, BitMap * bmap, int coverageIndex /*= -1*/)
{
	nvCheck(passCount > 0);
	nvCheck(img != NULL);
	nvCheck(bmap != NULL);

	const int w = img->width();
	const int h = img->height();
	const int count = img->componentNum();

	nvCheck(bmap->width() == uint(w));
	nvCheck(bmap->height() == uint(h));

	AutoPtr<BitMap> newbmap( new BitMap(w, h) );

	float * coverageChannel = NULL;
	if (coverageIndex != -1)
	{
		coverageChannel = img->channel(coverageIndex);
	}

	int firstChannel = -1;

	for (int p = 0; p < passCount; p++)
	{
		for (int c = 0; c < count; c++)
		{
			if (c == coverageIndex) continue;
			if (firstChannel == -1) firstChannel = c;

			float * channel = img->channel(c);
			
			for (int yb = 0; yb < h; yb++) {
				for (int xb = 0; xb < w; xb++) {
					
					if (bmap->bitAt(xb, yb)) {
						// Not a hole.
						newbmap->setBitAt(xb, yb);
						continue;
					}
					
					int numFill = 0;
					
					LocalPixels lp;
					for (int ny = 0; ny < 5; ny++)
					{
						int y = (yb + ny - 2);
						if ( y < 0 || y >= h )
						{
							// out of range
							for(int i = 0; i < 5; i++) 
							{
								lp.fill[ny][i] = false;
							}
							continue;
						}

						for (int nx = 0; nx < 5; nx++)
						{
							int x = (xb + nx - 2);
							if (x < 0 || x >= w)
							{
								lp.fill[ny][nx] = false;
							}
							else
							{
								int idx = img->index(x, y);
								if (!bmap->bitAt(idx))
								{
									lp.fill[ny][nx] = false;
								}
								else
								{
									lp.fill[ny][nx] = true;
									lp.data[ny][nx] = channel[idx];
									numFill++;
								}
							}
						}
					}

					// need at least 3 to do anything decent
					if (numFill < 2)
						continue;

					nvDebugCheck(lp.fill[2][2] == false);
					
					if (lp.doLocalPixelFill())
					{
						const int idx = img->index(xb, yb);
						channel[idx] = lp.result / lp.weight;

						if (c == firstChannel)
						{
							//coverageChannel[idx] /= lp.weight;	// @@ Not sure what this was for, coverageChannel[idx] is always zero.
							newbmap->setBitAt(xb, yb);
						}
					}
				}
			}
		}

		// Update the bit mask.
		swap(*newbmap, *bmap);
	}
}
