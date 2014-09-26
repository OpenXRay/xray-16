#ifndef XRS_H
#define XRS_H

#ifdef XRSPHERICAL_EXPORTS
#define XRS_API __declspec(dllexport)
#else
#define XRS_API __declspec(dllimport)
#endif
 
extern "C" {
 
	struct XRS_Sphere
	{	float x,y,z,r; }; 
	XRS_API void __cdecl xrSphere_Minimal		 (void* data, int count, XRS_Sphere& S);
	XRS_API void __cdecl xrSphere_Minimal_Ptr	 (void* data, int count, XRS_Sphere& S);
};

#endif