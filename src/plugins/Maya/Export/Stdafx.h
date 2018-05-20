#pragma once

#include <algorithm>

#include "Common/Common.hpp"
#include "Common/FSMacros.hpp"

#define ENGINE_API
#include "xrCore/xrCore.h"
#include "xrCore/_fbox.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <assert.h>

using std::min;
using std::max;

#define REQUIRE_IOSTREAM
#define _BCL
#include <maya/MPxLocatorNode.h>
#include <maya/MFnTransform.h>
#include <maya/MEulerRotation.h>
#include <maya/MStatus.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MArgList.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MPoint.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnPlugin.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshEdge.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MObjectArray.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MAnimControl.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MFnDagNode.h>
#include <maya/MItDag.h>
#include <maya/MDistance.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnCamera.h>
#include <maya/MAnimControl.h>

#include <iostream>
#include <maya/MSceneMessage.h>

#pragma warning(pop)

#define ENGINE_API
#define ECORE_API

enum TMsgDlgType
{
    mtWarning,
    mtError,
    mtInformation,
    mtConfirmation,
    mtCustom
};
enum TMsgDlgBtn
{
    mbYes,
    mbNo,
    mbOK,
    mbCancel,
    mbAbort,
    mbRetry,
    mbIgnore,
    mbAll,
    mbNoToAll,
    mbYesToAll,
    mbHelp
};
typedef TMsgDlgBtn TMsgDlgButtons[mbHelp];

#pragma warning(push)
#pragma warning(disable : 4995)
#include "plugins/Shared/ELog.h"
#pragma warning(pop)

#include "xrCore/Animation/Bone.hpp"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>
#pragma warning(pop)

#include <time.h>

#include <string>

//#define AnsiString std::string
#define AnsiString xr_string
using AStringVec = xr_vector<xr_string>;

#define THROW R_ASSERT(0)

#ifdef _MAYA_EXPORT
#define _EDITOR_FILE_NAME_ "maya_export"
#else
#ifdef _MAYA_MATERIAL
#define _EDITOR_FILE_NAME_ "maya_material"
#endif
#endif

#define GAMEMTL_NONE u32(-1)

#pragma comment(lib, "d3dx9.lib")
