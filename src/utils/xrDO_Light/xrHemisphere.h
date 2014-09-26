#ifndef xrHemisphereH
#define xrHemisphereH

#pragma once

#ifdef XRHEMISPHERE_EXPORTS
#define XRHS_API __declspec(dllexport)
#else
#define XRHS_API __declspec(dllimport)
#endif

typedef void __stdcall		xrHemisphereIterator(float x, float y, float z, float energy, LPVOID param);

extern "C"
{
	// Returns TRUE only if everything OK.
	XRHS_API void	xrHemisphereBuild
		(
			int						quality,
			BOOL					ground,
			float					ground_scale,
			float					energy,
			xrHemisphereIterator*	it,
			LPVOID					param
		);
};

#endif //xrHemisphereH
