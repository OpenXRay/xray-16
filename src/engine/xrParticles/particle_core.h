//---------------------------------------------------------------------------
#ifndef particle_coreH
#define particle_coreH

#define SQRT2PI 2.506628274631000502415765284811045253006f
#define ONEOVERSQRT2PI (1.f/SQRT2PI)

namespace PAPI{
	#pragma pack (push,4)
	struct PARTICLES_API pDomain
	{
		PDomainEnum type;			// PABoxDomain, PASphereDomain, PAConeDomain...
		pVector 	p1, p2;			// Box vertices, Sphere center, Cylinder/Cone ends
		pVector 	u, v;			// Orthonormal basis vectors for Cylinder/Cone
		float 		radius1;		// Outer radius
		float 		radius2;		// Inner radius
		float 		radius1Sqr;		// Used for fast Within test of spheres,
		float 		radius2Sqr;		// and for mag. of u and v vectors for plane.

		BOOL 		Within			(const pVector &) const;
		void 		Generate		(pVector &) const;
		// transformation
		void 		transform		(const pDomain& domain, const Fmatrix& m);
		void 		transform_dir	(const pDomain& domain, const Fmatrix& m);
		// This constructor is used when default constructing a
		// ParticleAction that has a pDomain.
		IC 			pDomain			(){}
		// Construct a domain in the standard way.
					pDomain			(PDomainEnum dtype,
                                    float a0=0.0f, float a1=0.0f, float a2=0.0f,
                                    float a3=0.0f, float a4=0.0f, float a5=0.0f,
                                    float a6=0.0f, float a7=0.0f, float a8=0.0f);
	};
	#pragma pack (pop)
	// misc    
	float 			NRand			(float sigma = 1.0f);
};
//---------------------------------------------------------------------------
#endif
 