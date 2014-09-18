#ifndef perlinH
#define perlinH

#define SAMPLE_SIZE 256

class ENGINE_API CPerlinNoiseCustom
{
protected:
	int		mSeed;
	bool	mReady;

	int		p					[SAMPLE_SIZE + SAMPLE_SIZE + 2];
protected:
	int		mOctaves;
	float	mFrequency;
	float	mAmplitude;
	xr_vector<float> mTimes;
public:
			CPerlinNoiseCustom	(int seed):
								mOctaves(2),mFrequency(1),mAmplitude(1),mSeed(seed),mReady(false){}
	IC void	SetParams			(int oct, float freq, float amp)
	{
		mOctaves				= oct;
		mFrequency				= freq;
		mAmplitude				= amp;
	}
	IC void	SetOctaves			(int oct)	{mOctaves	= oct; mTimes.resize(mOctaves);}
	IC void	SetFrequency		(float freq){mFrequency	= freq;}
	IC void	SetAmplitude		(float amp)	{mAmplitude	= amp;}
};

class ENGINE_API CPerlinNoise1D: public CPerlinNoiseCustom
{
private:
	float	noise				(float arg);
	float	g1					[SAMPLE_SIZE + SAMPLE_SIZE + 2];

	void	init				();
	float	mPrevContiniousTime;
public:
			CPerlinNoise1D		(int seed):CPerlinNoiseCustom(seed){mPrevContiniousTime=0.0f;}
	float	Get					(float x);
	float	GetContinious		(float v);
};

class ENGINE_API CPerlinNoise2D: public CPerlinNoiseCustom
{
private:
	float	noise				(const Fvector2& vec);
	void	normalize			(float v[2]);

	float	g2					[SAMPLE_SIZE + SAMPLE_SIZE + 2][2];

	void	init				();
public:
			CPerlinNoise2D		(int seed):CPerlinNoiseCustom(seed){}
	float	Get					(float x, float y);
};

class ENGINE_API CPerlinNoise3D: public CPerlinNoiseCustom
{
private:
	float	noise				(const Fvector3& vec);
	void	normalize			(float v[3]);

	float	g3					[SAMPLE_SIZE + SAMPLE_SIZE + 2][3];

	void	init				();
public:
			CPerlinNoise3D		(int seed):CPerlinNoiseCustom(seed){}
	float	Get					(float x, float y, float z);
};

#endif

