#ifndef XRayMtlH
#define XRayMtlH

///////////////////////////////////////////////////////////////////
//
// NOTE: PLEASE READ THE README.TXT FILE FOR INSTRUCTIONS ON
// COMPILING AND USAGE REQUIREMENTS.
//
// DESCRIPTION: Simple hardware shader which uses regular OpenGL
//				texturing and disables lighting.
//
//	This sample demonstrates how to:
//		- Specify a color attribute on your HW shader plug-in 
//		  so that end-users can attach a file texture to it.
//		- Traverse what's upstream of the color attribute, including 
//		  going through shading switches if required.
//		- Using the MImage API class to load and uncompress a file 
//		  texture in any Maya-supported file format.
//		- Setting up the graphics pipeline to display the 
//		  texture correctly.
//
//
///////////////////////////////////////////////////////////////////

#include <maya/MPxHwShaderNode.h>
#include "MTextureCache.h"

class CXRayMtl : public MPxHwShaderNode
{
public:
                    CXRayMtl();
    virtual         ~CXRayMtl();
	void			releaseEverything();

    virtual MStatus compute( const MPlug&, MDataBlock& );
	virtual void      postConstructor();

	virtual MStatus	bind(const MDrawRequest& request,
						 M3dView& view);

	virtual MStatus	unbind(const MDrawRequest& request,
						   M3dView& view);

	virtual MStatus	geometry( const MDrawRequest& request,
							  M3dView& view,
							  int prim,
							  unsigned int writable,
							  int indexCount,
							  const unsigned int * indexArray,
							  int vertexCount,
							  const int * vertexIDs,
							  const float * vertexArray,
							  int normalCount,
							  const float ** normalArrays,
							  int colorCount,
							  const float ** colorArrays,
							  int texCoordCount,
							  const float ** texCoordArrays);

	virtual int		normalsPerVertex();
	virtual bool	hasTransparency();
	virtual int		texCoordsPerVertex();

    static  void *  creator();
    static  MStatus initialize();
    static  MTypeId id;

	MTextureCache*	m_pTextureCache;

	MStatus			getFloat3(MObject colorAttr, float colorValue[3]);
	MStatus			getString(MObject attr, MString &str);

	void			updateTransparencyFlags(MString objectPath);

protected:
    static MObject  colorR;
	static MObject  colorG;
	static MObject  colorB;
	static MObject  color;

	static MObject	xrEngineData;
	static MObject	xrCompilerData;
	static MObject	xrMaterialData;
	static MObject	xrDoubleSide;

	static MObject  transparencyR;
	static MObject  transparencyG;
	static MObject  transparencyB;
	static MObject  transparency;
	float fConstantTransparency;

	MDagPath currentObjectPath;

	// Callbacks that we monitor so we can release OpenGL-dependant resources before
	// their context gets destroyed.
	MCallbackId fBeforeNewCB;
	MCallbackId fBeforeOpenCB;
	MCallbackId fBeforeRemoveReferenceCB;
	MCallbackId fMayaExitingCB;

	void attachSceneCallbacks();
	void detachSceneCallbacks();

	static void releaseCallback(void* clientData);
};

#endif /* XRayMtlH */
