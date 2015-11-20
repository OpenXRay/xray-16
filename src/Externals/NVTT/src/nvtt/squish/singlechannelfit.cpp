/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
	Copyright (c) 2006 Ignacio Castano                      castanyo@yahoo.es

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
#include "singlechannelfit.h"
#include "colourset.h"
#include "colourblock.h"
#include <cfloat>

namespace squish {

SingleChannelFit::SingleChannelFit( ColourSet const* colours, int const flags ) 
  : ColourFit( /*colours, flags*/ )
{
	SetColourSet( colours, flags );
	// cache some values
	unsigned int const count = m_colours->GetCount();
	Vec3 const* values = m_colours->GetPoints();
	
	// Find bounds of the search space.
	m_g_min = 63;
	m_g_max = 0;
	
	for(unsigned int i = 0; i < count; i++) {
		
		int grey = int(values[i].Y() * 255.0f);	// @@ rounding?
		grey = std::min(grey, 255);	// clamp to [0, 1)
		grey = std::max(grey, 0);
		m_greys[i] = u8(grey);
		
		m_g_min = std::min(m_g_min, grey >> 2);
		m_g_max = std::max(m_g_max, grey >> 2);
	}
	
	int const g_pad = m_g_max - m_g_min + 1;

	m_g_min = std::max(0, m_g_min - g_pad);
	m_g_max = std::min(63, m_g_max + g_pad);
}

void SingleChannelFit::Compress3( void* block )
{
	// do not do anything.
}

void SingleChannelFit::Compress4( void* block )
{
	int const count = m_colours->GetCount();
	Vec3 const* values = m_colours->GetPoints();
	float const* weights = m_colours->GetWeights();
	
	int best_g0;
	int best_g1;
	float best_error = FLT_MAX;
	
	// Brute force approach, try all the possible endpoints with g0 > g1.
	for(int g0 = m_g_min+1; g0 <= m_g_max; g0++) {
		for(int g1 = m_g_min; g1 < g0; g1++) {
			
			// Compute palette.
			const int c0 = (g0 << 2) | (g0 >> 4);
			const int c1 = (g1 << 2) | (g1 >> 4);
			const int c2 = (2 * c0 + c1) / 3;
			const int c3 = (2 * c1 + c0) / 3;
			
			// Evaluate palette error.
			float error = 0;
			for(int i = 0; i < count; i++) {
				const int grey = m_greys[i];
				
				int min_dist = abs(c0 - grey);	// Use absolute distance, not squared.
				min_dist = std::min(min_dist, abs(c1 - grey));
				min_dist = std::min(min_dist, abs(c2 - grey));
				min_dist = std::min(min_dist, abs(c3 - grey));
				
				error += min_dist * weights[i];
			}
			
			if(error < best_error) {
				best_error = error;
				best_g0 = g0;
				best_g1 = g1;
			}
		}
	}
	
	// Compute best palette.
	const int best_c0 = (best_g0 << 2) | (best_g0 >> 4);
	const int best_c1 = (best_g1 << 2) | (best_g1 >> 4);
	const int best_c2 = (2 * best_c0 + best_c1) / 3;
	const int best_c3 = (2 * best_c1 + best_c0) / 3;
	
	// Compute best indices.
	u8 closest[16];
	for(int i = 0; i < count; i++) {
		const int grey = m_greys[i];
		
		int dist = abs(best_c0 - grey);
		int min_dist = dist;
		int min_i = 0;
		
		dist = abs(best_c1 - grey);
		if( dist < min_dist ) { min_dist = dist; min_i = 1; }
		
		dist = abs(best_c2 - grey);
		if( dist < min_dist ) { min_dist = dist; min_i = 2; }
		
		dist = abs(best_c3 - grey);
		if( dist < min_dist ) { min_dist = dist; min_i = 3; }
		
		closest[i] = min_i;
	}
	
	// remap the indices
	u8 indices[16];
	m_colours->RemapIndices( closest, indices );
	
	// Output block.
	WriteColourBlock(best_g0 << 5, best_g1 << 5, indices, block);
}


} // namespace squish
