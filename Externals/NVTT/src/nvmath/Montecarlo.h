// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_MONTECARLO_H
#define NV_MATH_MONTECARLO_H

#include <nvmath/Vector.h>
#include <nvmath/Random.h>

namespace nv
{

/// A random sample distribution.
class SampleDistribution
{
public:
	
	// Sampling method.
	enum Method {
		Method_Random,
		Method_Stratified,
		Method_NRook
	};

	// Distribution functions.
	enum Distribution {
		Distribution_Uniform,
		Distribution_Cosine
	};
	
	/// Constructor.
	SampleDistribution(int num)
	{
		m_sampleArray.resize(num);
	}
	
	void redistribute(Method method=Method_NRook, Distribution dist=Distribution_Cosine);
	
	/// Get parametric coordinates of the sample.
	Vector2 sample(int i) { return m_sampleArray[i].uv; }
	
	/// Get sample direction.
	Vector3 sampleDir(int i) { return m_sampleArray[i].dir; }

	/// Get number of samples.
	uint sampleCount() const { return m_sampleArray.count(); }
	
private:
	
	void redistributeRandom(const Distribution dist);
	void redistributeStratified(const Distribution dist);
	void multiStageNRooks(const int size, int* cells);
	void redistributeNRook(const Distribution dist);
	
	
	/// A sample of the random distribution.
	struct Sample
	{
		/// Set sample given the 3d coordinates.
		void setDir(float x, float y, float z) {
			dir.set(x, y, z);
			uv.set(acosf(z), atan2f(y, x));
		}
		
		/// Set sample given the 2d parametric coordinates.
		void setUV(float u, float v) {
			uv.set(u, v);
			dir.set(sinf(u) * cosf(v), sinf(u) * sinf(v), cosf(u));
		}
		
		Vector2 uv;
		Vector3 dir;
	};
	
	/// Random seed.
	MTRand m_rand;
	
	/// Samples.
	Array<Sample> m_sampleArray;
	
};

} // nv namespace

#endif // NV_MATH_MONTECARLO_H
