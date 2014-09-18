#ifndef ExportDefinesH
#define ExportDefinesH

// refs
class CSurface;

//
// Edge info structure
//
typedef struct SXREdgeInfo {
	int                 polyIds[2]; // Id's of polygons that reference edge
	int                 vertId;     // The second vertex of this edge
	struct SXREdgeInfo* next;       // Pointer to next edge
	bool                smooth;     // Is this edge smooth
} * SXREdgeInfoPtr;

struct SXRShaderData
{
	MString		name;
	MString		tex_name;
	MString		eng_name;
	MString		comp_name;
	MString		gmat_name;
	BOOL		double_side;
	SXRShaderData():double_side(FALSE),name(""),tex_name(""),eng_name("default"),comp_name("default"),gmat_name("default"){};
};
DEFINE_VECTOR(SXRShaderData,XRShaderDataVec,XRShaderDataIt);

extern MStatus	parseShader	(MObject &src, SXRShaderData& d);
extern MObject	findShader	(MObject& setNode, SXRShaderData& d);
#endif