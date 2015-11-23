// This code is in the public domain -- castanyo@yahoo.es

#include <nvmath/Montecarlo.h>

using namespace nv;


void SampleDistribution::redistribute(Method method/*=Method_NRook*/, Distribution dist/*=Distribution_Cosine*/)
{
	switch(method) 
	{
		case Method_Random:
			redistributeRandom(dist);
			break;
		case Method_Stratified:
			redistributeStratified(dist);
			break;
		case Method_NRook:
			redistributeNRook(dist);
			break;
	};
}
	
void SampleDistribution::redistributeRandom(const Distribution dist)
{
	const uint sampleCount = m_sampleArray.count();
	
	// This is the worst method possible!
	for(uint i = 0; i < sampleCount; i++)
	{
		float x = m_rand.getFloat();
		float y = m_rand.getFloat();
		
		// Map uniform distribution in the square to the (hemi)sphere.
		if( dist == Distribution_Uniform ) {
			m_sampleArray[i].setUV(acosf(1 - 2 * x), 2 * PI * y);
		}
		else {
			nvDebugCheck(dist == Distribution_Cosine);
			m_sampleArray[i].setUV(acosf(sqrtf(x)), 2 * PI * y);
		}
	}
}


void SampleDistribution::redistributeStratified(const Distribution dist)
{
	const uint sampleCount = m_sampleArray.count();
	const uint sqrtSampleCount = uint(sqrtf(float(sampleCount)));
	
	nvDebugCheck(sqrtSampleCount*sqrtSampleCount == sampleCount);	// Must use exact powers!

	// Create a uniform distribution of points on the hemisphere with low variance.
	for(uint v = 0, i = 0; v < sqrtSampleCount; v++) {
		for(uint u = 0; u < sqrtSampleCount; u++, i++) {
			float x = (u + m_rand.getFloat()) / float(sqrtSampleCount);
			float y = (v + m_rand.getFloat()) / float(sqrtSampleCount);
			
			// Map uniform distribution in the square to the (hemi)sphere.
			if( dist == Distribution_Uniform ) {
				m_sampleArray[i].setUV(acosf(1 - 2 * x), 2 * PI * y);
			}
			else {
				nvDebugCheck(dist == Distribution_Cosine);
				m_sampleArray[i].setUV(acosf(sqrtf(x)), 2 * PI * y);
			}
		}
	}
}


/** Multi-Stage N-rooks Sampling Method.
 * See: http://www.acm.org/jgt/papers/WangSung9/9
 */
void SampleDistribution::multiStageNRooks(const int size, int* cells)
{
	if (size == 1) {
		return;
	}

	int size1 = size >> 1;
	int size2 = size >> 1;

	if (size & 1) {
		if (m_rand.getFloat() > 0.5) {
			size1++;
		}
		else {
			size2++;
		}
	}

	int* upper_cells = new int[size1];
	int* lower_cells = new int[size2];

	int i, j;
	for(i = 0, j = 0; i < size - 1; i += 2, j++) {
		if (m_rand.get() & 1) {
			upper_cells[j] = cells[i];
			lower_cells[j] = cells[i + 1];
		}
		else {
			upper_cells[j] = cells[i + 1];
			lower_cells[j] = cells[i];
		}
	}

	if (size1 != size2) {
		if (size1 > size2) {
			upper_cells[j] = cells[i];
		}
		else {
			lower_cells[j] = cells[i];
		}
	}

	multiStageNRooks(size1, upper_cells);
	memcpy(cells, upper_cells, size1 * sizeof(int));
	delete [] upper_cells;

	multiStageNRooks(size2, lower_cells);
	memcpy(cells + size1, lower_cells, size2 * sizeof(int));
	delete [] lower_cells;
}


void SampleDistribution::redistributeNRook(const Distribution dist)
{
	const uint sampleCount = m_sampleArray.count();
	
	// Generate nrook cells
	int * cells = new int[sampleCount];
	for(uint32 i = 0; i < sampleCount; i++)
	{
		cells[i] = i;
	}
	multiStageNRooks(sampleCount, cells);

	for(uint i = 0; i < sampleCount; i++)
	{
		float x = (i + m_rand.getFloat()) / sampleCount;
		float y = (cells[i] + m_rand.getFloat()) / sampleCount;

		// Map uniform distribution in the square to the (hemi)sphere.
		if( dist == Distribution_Uniform ) {
			m_sampleArray[i].setUV(acosf(1 - 2 * x), 2 * PI * y);
		}
		else {
			nvDebugCheck(dist == Distribution_Cosine);
			m_sampleArray[i].setUV(acosf(sqrtf(x)), 2 * PI * y);
		}
	}

	delete [] cells;
}

