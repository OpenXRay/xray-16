#include "stdafx.h"
#pragma hdrstop

#include "SmdMaya.h"
#include "maTranslator.h"
#include "camera_exporter.h"

const char* const xrayObjectOptionScript = "xrayObjectExportOptions";
const char* const xrayObjectDefaultOptions = 0;
const char* const xrayCameraMotionDefaultOptions = 0;

const char* const xraySkinOptionScript = "xraySkinExportOptions";
const char* const xraySkinDefaultOptions = "SkinCluster=;";

const char* const xraySkinMotionOptionScript = "xraySkinMotionExportOptions";
const char* const xraySkinMotionDefaultOptions = "SkinCluster=;";

MStatus uninitializePlugin(MObject obj)
{
    MFnPlugin plugin(obj);
    MStatus status;
    plugin.deregisterFileTranslator("XRay_Object_Export");
    plugin.deregisterFileTranslator("XRay_Skin_Export");
    plugin.deregisterFileTranslator("XRay_Skin_Motion_Export");

    Core._destroy();

    return status;
}
//////////////////////////////////////////////////////////////

static MObject INIT_OBJ = MObject::kNullObj;

void uninitialize(void*)
{
    if (!INIT_OBJ.isNull())
    {
        uninitializePlugin(INIT_OBJ);
        INIT_OBJ = MObject::kNullObj;
    }
    Core._destroy();
}

MStatus initializePlugin(MObject obj)
{
    INIT_OBJ = obj;
    Core.Initialize("XRayMayaPlugin", nullptr, LogCallback(ELogCallback, nullptr), FALSE, nullptr, true);
    FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");

    MFnPlugin plugin(obj, "GSC Game World", "1.00", "Any");

    // add callbacks
    //	MSceneMessage::addCallback(MSceneMessage::kMayaExiting,  uninitialize, 0);

    MStatus status;
    // Register the translator with the system
    status = plugin.registerFileTranslator("XRay_Object_Export", "none", CXRayObjectExport::creator,
        (char*)xrayObjectOptionScript, (char*)xrayObjectDefaultOptions);
    if (status != MS::kSuccess)
        return status;
    status = plugin.registerFileTranslator("XRay_Skin_Export", "none", CXRaySkinExport::creator_skin,
        (char*)xraySkinOptionScript, (char*)xraySkinDefaultOptions);
    if (status != MS::kSuccess)
        return status;
    status = plugin.registerFileTranslator("XRay_Skin_Motion_Export", "none", CXRaySkinExport::creator_skin_motion,
        (char*)xraySkinMotionOptionScript, (char*)xraySkinMotionDefaultOptions);
    if (status != MS::kSuccess)
        return status;
    status = plugin.registerFileTranslator("XRay_Camera_Motion_Export", "none", CXRayCameraExport::creator,
        (char*)xraySkinMotionOptionScript, (char*)xrayCameraMotionDefaultOptions);
    if (status != MS::kSuccess)
        return status;

    return status;
}
//////////////////////////////////////////////////////////////
