#ifndef xrHemisphereH
#define xrHemisphereH

typedef void __stdcall		xrHemisphereIterator(float x, float y, float z, float energy, LPVOID param);

void	ECORE_API	xrHemisphereBuild		(int quality, float energy, xrHemisphereIterator* it, LPVOID param);
int		ECORE_API	xrHemisphereVertices	(int quality, const Fvector*& verts);
int		ECORE_API	xrHemisphereIndices		(int quality, const u16*& indices);

#endif //xrHemisphereH
