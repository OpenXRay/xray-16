#ifndef __XR_GAMMA_H__
#define __XR_GAMMA_H__

//-----------------------------------------------------------------------------------------------------------
//Gamma control
//-----------------------------------------------------------------------------------------------------------
class					CGammaControl
{
	float						fGamma;
	float						fBrightness;
	float						fContrast;
	Fcolor						cBalance;

public:
	CGammaControl		() :
	  fGamma(1.f)
	{ Brightness(1.f); Contrast(1.f); Balance(1.f,1.f,1.f); };

	IC void	Balance		(float _r, float _g, float _b)
	{	cBalance.set	(_r,_g,_b,1);	}
	IC void	Balance		(Fcolor &C)
	{	Balance(C.r,C.g,C.b); }

	IC void Gamma		(float G) { fGamma		= G;	}
	IC void Brightness	(float B) { fBrightness = B;	}
	IC void Contrast	(float C) { fContrast	= C;	}

	void	GetIP		(float& G, float &B, float& C, Fcolor& Balance)
	{
		G			= fGamma;
		B			= fBrightness;
		C			= fContrast;
		Balance.set	(cBalance);
	}

	void	Update		();

private:

#if defined(USE_DX10) || defined(USE_DX11)
	void	GenLUT		(const DXGI_GAMMA_CONTROL_CAPABILITIES &GC, DXGI_GAMMA_CONTROL &G);
#else	//	USE_DX10
	void	GenLUT		(D3DGAMMARAMP &G);
#endif	//	USE_DX10
};

#endif
