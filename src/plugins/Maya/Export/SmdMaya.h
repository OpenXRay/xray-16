#ifndef SmdMayaH
#define SmdMayaH

#include "..\..\..\editors\Ecore\editor\EditMesh.H"
#include "ExportDefines.H"
#include "SmdBone.h"
#include "SmdTriangle.h"

// refs
class CEditableObject;

struct lstr {
	bool operator() (char *s1, char *s2) const
	{
		return (xr_strcmp(s1, s2)< 0);
	}
};

typedef xr_map<char *, int, struct lstr> NameIntMap;
typedef xr_map<int, int> PtLookupMap;

DEFINE_VECTOR(SmdBone*,		SmdBoneVec,	SmdBoneIt);
DEFINE_VECTOR(SmdTriangle*,	SmdTriVec,	SmdTriIt);
DEFINE_VECTOR(SmdVertex*,	SmdVertVec,	SmdVertIt);

class CXRaySkinExport: public MPxFileTranslator
{
public:
					CXRaySkinExport			(bool bReference);
	virtual			~CXRaySkinExport		();

	// these functions are all used to initialize this as a Maya
	// file exporter plug-in
	static void*	creator_skin			();
	static void*	creator_skin_motion		();
	MStatus			reader					(const MFileObject& file, const MString& optionsString, FileAccessMode mode);
	MStatus			writer					(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

	bool			haveWriteMethod			() const ;
	virtual MString			filter					() const;
	//virtual MString			defaultExtension		() const;
	MFileKind		identifyFile			(const MFileObject& fileName, const char* buffer, short size) const;
private:
	// these functions are all used to iterate across geometry, filling data structures
	MStatus			gotoBindPose			(void);
	MStatus			getBones				(void);	// loads geometry from scene
	MStatus			getBoneData				(const MMatrix& locator);	// loads bone position per animation frame
	MStatus			setUpBoneMap			(MObject* pSkinObject);	// creates data structure to store data
	void			parseMesh				(MObject &mesh, MDagPath path);
	MStatus			parseBoneGeometry		();
	MStatus			parseShape				(MDagPath path);
	MStatus			parsePolySet			(MItMeshPolygon &meshPoly, MObjectArray& rgShaders, MIntArray texMap, int weight);
	MStatus			CalculateTriangleVertex (int vt, MVector &pt, float &u, float &v, WBVec& weights, MItMeshPolygon &meshPoly, PtLookupMap &ptMap);
	void			clearData				(void);	// call inbetween function calls
//	MStatus			GetShaderFileName		(MString &filename, MObject &set);
	int				AppendVertex			(MPoint pt, float u, float v, const WBVec& wb);

	// Edge lookup methods
	//
	void            buildEdgeTable			(MDagPath&);
	void            CreateSmoothingGroups	( MFnMesh& );
	void            CreateSMGFacegroups		( MFnMesh& );
	void            CreateSMGEdgeAttrs		( MFnMesh& );
	void            addEdgeInfo			(int, int, bool);

public:
	SXREdgeInfoPtr  findEdgeInfo		(int, int);

private:
	void            destroyEdgeTable	();
	bool            smoothingAlgorithm	(int, MFnMesh&);
	// Edge lookup table (by vertex id) and smoothing group info
	//
	SXREdgeInfoPtr*	edgeTable;
	int *           polySmoothingGroups;
	int             edgeTableSize;
	int             nextSmoothingGroup;
	int             currSmoothingGroup;
	bool            newSmoothingGroup;

	// functions for printing out the animation data into files
	MStatus			exportObject			(LPCSTR fname, bool b_ogf);
	MStatus			exportMotion			(LPCSTR fname, const MMatrix& locator);

	// these are various data structures used to store scene geometry
	NameIntMap		m_jointMap;
	SmdBoneVec		m_boneList;
	SmdTriVec		m_triList;
	SmdVertVec		m_vertList;
	MDagPath		m_skinPath;			// path to the mesh we discover
	MDagPathArray	m_rgInfs;			// array of paths to the bones we discover

	VWBVec*			m_rgWeights;		// for each vertex, store index of influence joint
	MString			m_strFilename;		// filename for file

	// options used to drive export behavior
	bool			m_bReferenceFile;	// true for a reference file, false for an animation file
	bool			m_fSkinCluster;		// true if we should only export the cluster named by "m_strSkinCluster"
	MString			m_strSkinCluster;	// name of skin cluster selected by user to be exported. if empty, use first cluster in list
};

#endif //SmdMayaH