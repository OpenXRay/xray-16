//+
// Copyright (C) Alias Systems,  a division  of  Silicon Graphics Limited and/or
// its licensors ("Alias").  All rights reserved.  These coded instructions,
// statements, computer programs, and/or related material (collectively, the
// "Material") contain unpublished information proprietary to Alias, which is
// protected by Canadian and US federal copyright law and by international
// treaties.  This Material may not be disclosed to third parties, or be copied
// or duplicated, in whole or in part, without the prior written consent of
// Alias.  ALIAS HEREBY DISCLAIMS ALL WARRANTIES RELATING TO THE MATERIAL,
// INCLUDING, WITHOUT LIMITATION, ANY AND ALL EXPRESS OR IMPLIED WARRANTIES OF
// NON-INFRINGEMENT, MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
// IN NO EVENT SHALL ALIAS BE LIABLE FOR ANY DAMAGES WHATSOEVER, WHETHER DIRECT,
// INDIRECT, SPECIAL, OR PUNITIVE, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, OR IN EQUITY, ARISING OUT OF OR RELATED TO THE
// ACCESS TO, USE OF, OR RELIANCE UPON THE MATERIAL.
//-

// Example Plugin: lambertShader.cpp
//
// Produces dependency graph node LambertShader
// This node is an example of a Lambert shader and how to build a
// dependency node as a surface shader in Maya. The inputs for this node
// are many, and can be found in the Maya UI on the Attribute Editor for
// the node. The output attributes for the node are "outColor" and
// "outTransparency". To use this shader, create a lambertShader with
// Shading Group or connect the outputs to a Shading Group's
// "SurfaceShader" attribute.
//
#include "stdafx.h"
#pragma hdrstop

#include <maya/MFnStringData.h>
#include <maya/MFnNumericAttribute.h>
#include "plugins/Shared/BlenderListLoader.h"
using namespace std;
/////////////////////////////////
// Plugin Lambert Shader Class //
/////////////////////////////////

// This class will create a new shader. Shaders are custom dependency
// graph objects so we will derive this class from the basic DG node
// type MPxNode
//

class CXRayMtl : public MPxNode
{
public:
    CXRayMtl();
    virtual ~CXRayMtl();

    static void* creator();
    virtual MStatus compute(const MPlug&, MDataBlock&);
    static MStatus initialize();

    // postConstructor:
    // The postConstructor method allows us to call MPxNode member
    // functions during initialization. Internally maya creates two
    // objects when a user defined node is created, the internal MObject
    // and the user derived object. The association between the these
    // two objects is not made until after the MPxNode constructor is
    // called. This implies that no MPxNode member function can be called
    // from the MPxNode constructor. The postConstructor will get called
    // immediately after the constructor when it is safe to call any
    // MPxNode member function.
    //

    virtual void postConstructor();

    static MTypeId id; // The IFF type id

protected:
    // Translucence coefficient
    static MObject aTranslucenceCoeff;

    // Diffuse Reflectivity
    static MObject aDiffuseReflectivity;

    // Red component of surface color
    static MObject aColorR;

    // Green component of surface color
    static MObject aColorG;

    // Blue component of surface color
    static MObject aColorB;

    // Surface color
    static MObject aColor;

    // Red component of incandescence
    static MObject aIncandescenceR;

    // Green component of incandescence
    static MObject aIncandescenceG;

    // Blue component of incandescence
    static MObject aIncandescenceB;

    // Incandescence
    static MObject aIncandescence;

    // Red component of surface transparency
    static MObject aInTransR;

    // Green component of surface transparency
    static MObject aInTransG;

    // Blue component of surface transparency
    static MObject aInTransB;

    // Surface transparency
    static MObject aInTransparency;

    // Red component of output color
    static MObject aOutColorR;

    // Green component of output color
    static MObject aOutColorG;

    // Blue component of output color
    static MObject aOutColorB;

    // Output color
    static MObject aOutColor;

    // Red component of output transparency
    static MObject aOutTransR;

    // Green component of output transparency
    static MObject aOutTransG;

    // Blue component of output transparency
    static MObject aOutTransB;

    // Output transparency
    static MObject aOutTransparency;

    // X component of surface normal
    static MObject aNormalCameraX;

    // Y component of surface normal
    static MObject aNormalCameraY;

    // Z component of surface normal
    static MObject aNormalCameraZ;

    // Surface normal
    static MObject aNormalCamera;

    // X component of light direction vector
    static MObject aLightDirectionX;

    // Y component of light direction vector
    static MObject aLightDirectionY;

    // Z component of light direction vector
    static MObject aLightDirectionZ;

    // Light direction vector
    static MObject aLightDirection;

    // Red component of light intensity
    static MObject aLightIntensityR;

    // Green component of light intensity
    static MObject aLightIntensityG;

    // Blue component of light intensity
    static MObject aLightIntensityB;

    // Light Intensity vector
    static MObject aLightIntensity;

    // Ambient flag
    static MObject aLightAmbient;

    // Diffuse flag
    static MObject aLightDiffuse;

    // Specular flag
    static MObject aLightSpecular;

    // Shadow Fraction flag
    static MObject aLightShadowFraction;

    // Pre Shadow Intensity
    static MObject aPreShadowIntensity;

    // Light blind data
    static MObject aLightBlindData;

    // Light data array
    static MObject aLightData;

    // XRay data
    static MObject xrEngineData;
    static MObject xrCompilerData;
    static MObject xrMaterialData;
    static MObject xrDoubleSide;
};

// IFF type ID
// Each node requires a unique identifier which is used by
// MFnDependencyNode::create() to identify which node to create, and by
// the Maya file format.
//
// For local testing of nodes you can use any identifier between
// 0x00000000 and 0x0007ffff, but for any node that you plan to use for
// more permanent purposes, you should get a universally unique id from
// Alias Support. You will be assigned a unique range that you
// can manage on your own.
//
MTypeId CXRayMtl::id(0x00105440);

// the postConstructor() function is called immediately after the objects
// constructor. It is not safe to call MPxNode member functions from the
// constructor, instead they should be called here.
//
void CXRayMtl::postConstructor()
{
    // setMPSafe indicates that this shader can be used for multiprocessor
    // rendering. For a shading node to be MP safe, it cannot access any
    // shared global data and should only use attributes in the datablock
    // to get input data and store output data.
    //
    setMPSafe(true);
}

///////////////////////////////////////////////////////
// DESCRIPTION: attribute information
///////////////////////////////////////////////////////
//
MObject CXRayMtl::aTranslucenceCoeff;
MObject CXRayMtl::aDiffuseReflectivity;
MObject CXRayMtl::aInTransparency;
MObject CXRayMtl::aInTransR;
MObject CXRayMtl::aInTransG;
MObject CXRayMtl::aInTransB;
MObject CXRayMtl::aColor;
MObject CXRayMtl::aColorR;
MObject CXRayMtl::aColorG;
MObject CXRayMtl::aColorB;
MObject CXRayMtl::aIncandescence;
MObject CXRayMtl::aIncandescenceR;
MObject CXRayMtl::aIncandescenceG;
MObject CXRayMtl::aIncandescenceB;
MObject CXRayMtl::aOutColor;
MObject CXRayMtl::aOutColorR;
MObject CXRayMtl::aOutColorG;
MObject CXRayMtl::aOutColorB;
MObject CXRayMtl::aOutTransparency;
MObject CXRayMtl::aOutTransR;
MObject CXRayMtl::aOutTransG;
MObject CXRayMtl::aOutTransB;
MObject CXRayMtl::aNormalCamera;
MObject CXRayMtl::aNormalCameraX;
MObject CXRayMtl::aNormalCameraY;
MObject CXRayMtl::aNormalCameraZ;
MObject CXRayMtl::aLightData;
MObject CXRayMtl::aLightDirection;
MObject CXRayMtl::aLightDirectionX;
MObject CXRayMtl::aLightDirectionY;
MObject CXRayMtl::aLightDirectionZ;
MObject CXRayMtl::aLightIntensity;
MObject CXRayMtl::aLightIntensityR;
MObject CXRayMtl::aLightIntensityG;
MObject CXRayMtl::aLightIntensityB;
MObject CXRayMtl::aLightAmbient;
MObject CXRayMtl::aLightDiffuse;
MObject CXRayMtl::aLightSpecular;
MObject CXRayMtl::aLightShadowFraction;
MObject CXRayMtl::aPreShadowIntensity;
MObject CXRayMtl::aLightBlindData;

// XRya data
MObject CXRayMtl::xrDoubleSide;
MObject CXRayMtl::xrEngineData;
MObject CXRayMtl::xrCompilerData;
MObject CXRayMtl::xrMaterialData;

// This node does not need to perform any special actions on creation or
// destruction
//

CXRayMtl::CXRayMtl() {}
CXRayMtl::~CXRayMtl() {}
// The creator() method allows Maya to instantiate instances of this node.
// It is called every time a new instance of the node is requested by
// either the createNode command or the MFnDependencyNode::create()
// method.
//
// In this case creator simply returns a new CXRayMtl object.
//

void* CXRayMtl::creator() { return new CXRayMtl; }
// The initialize method is called only once when the node is first
// registered with Maya. In this method you define the attributes of the
// node, what data comes in and goes out of the node that other nodes may
// want to connect to.
//

#define MAKE_INPUT(attr)                   \
    CHECK_MSTATUS(attr.setKeyable(true));  \
    CHECK_MSTATUS(attr.setStorable(true)); \
    CHECK_MSTATUS(attr.setReadable(true)); \
    CHECK_MSTATUS(attr.setWritable(true));

extern unsigned crc16_calc(unsigned char* data, unsigned count, unsigned old_crc = 0);

MStatus CXRayMtl::initialize()
{
    MFnNumericAttribute nAttr;
    MFnLightDataAttribute lAttr;
    MFnEnumAttribute eAttr;
    MFnTypedAttribute tAttr;

    MStatus status; // Status will be used to hold the MStatus value
    // returned by each api function call. It is important
    // to check the status returned by a call to aid in
    // debugging. Failed API calls can result in subtle
    // errors that can be difficult to track down, you may
    // wish to use the CHECK_MSTATUS macro for any API
    // call where you do not need to provide your own
    // error handling.
    //

    // Attribute Initialization:
    //
    // create      - The create function creates a new attribute for the
    //				 node, it takes a long name for the attribute, a short
    //				 name for the attribute, the type of the attribute,
    //				 and a status object to determine if the api call was
    //				 successful.
    //
    // setKeyable  - Sets whether this attribute should accept keyframe
    //				 data, Attributes are not keyable by default.
    //
    // setStorable - Sets whether this attribute should be storable. If an
    //				 attribute is storable, then it will be writen out
    //				 when the node is stored to a file. Attributes are
    //               storable by default.
    //
    // setDefault  - Sets the default value for this attribute.
    //
    // setUsedAsColor - Sets whether this attribute should be presented as
    //				 a color in the UI.
    //
    // setHidden   - Sets whether this attribute should be hidden from the
    //				 UI. This is useful if the attribute is being used for
    //				 blind data, or if it is being used as scratch space
    //				 for a geometry calculation (should also be marked
    //				 non-connectable in that case). Attributes are not
    //				 hidden by default.
    //
    // setReadable - Sets whether this attribute should be readable. If an
    //				 attribute is readable, then it can be used as the
    //				 source in a dependency graph connection. Attributes
    //				 are readable by default.
    //
    // setWritable - Sets whether this attribute should be readable. If an
    //				 attribute is writable, then it can be used as the
    //				 destination in a dependency graph connection. If an
    //			     attribute is not writable then setAttr commands will
    //				 fail to change the attribute. If both keyable and
    //				 writable for an attribute are set to true it will be
    //				 displayed in the channel box when the node is
    //				 selected. Attributes are writable by default.
    //
    // setArray    - Sets whether this attribute should have an array of
    //				 data. This should be set to true if the attribute
    //				 needs to accept multiple incoming connections.
    //				 Attributes are single elements by default.
    //

    // Input Attributes
    //
    aTranslucenceCoeff = nAttr.create("translucenceCoeff", "tc", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aDiffuseReflectivity = nAttr.create("diffuseReflectivity", "drfl", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.8f));

    aColorR = nAttr.create("colorR", "cr", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aColorG = nAttr.create("colorG", "cg", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.58824f));

    aColorB = nAttr.create("colorB", "cb", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.644f));

    aColor = nAttr.create("color", "c", aColorR, aColorG, aColorB, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f, 0.58824f, 0.644f));
    CHECK_MSTATUS(nAttr.setUsedAsColor(true));

    aIncandescenceR = nAttr.create("incandescenceR", "ir", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aIncandescenceG = nAttr.create("incandescenceG", "ig", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aIncandescenceB = nAttr.create("incandescenceB", "ib", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f));

    aIncandescence = nAttr.create("incandescence", "ic", aIncandescenceR, aIncandescenceG, aIncandescenceB, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f, 0.0f, 0.0f));
    CHECK_MSTATUS(nAttr.setUsedAsColor(true));

    aInTransR = nAttr.create("transparencyR", "itr", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));

    aInTransG = nAttr.create("transparencyG", "itg", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));

    aInTransB = nAttr.create("transparencyB", "itb", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));

    aInTransparency = nAttr.create("transparency", "it", aInTransR, aInTransG, aInTransB, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setKeyable(true));
    CHECK_MSTATUS(nAttr.setStorable(true));
    CHECK_MSTATUS(nAttr.setDefault(0.0f, 0.0f, 0.0f));
    CHECK_MSTATUS(nAttr.setUsedAsColor(true));

    // Output Attributes
    //

    // Color Output
    //
    aOutColorR = nAttr.create("outColorR", "ocr", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutColorG = nAttr.create("outColorG", "ocg", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutColorB = nAttr.create("outColorB", "ocb", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutColor = nAttr.create("outColor", "oc", aOutColorR, aOutColorG, aOutColorB, &status);
    CHECK_MSTATUS(status);

    CHECK_MSTATUS(nAttr.setHidden(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));

    // Transparency Output
    //
    aOutTransR = nAttr.create("outTransparencyR", "otr", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutTransG = nAttr.create("outTransparencyG", "otg", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutTransB = nAttr.create("outTransparencyB", "otb", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);

    aOutTransparency = nAttr.create("outTransparency", "ot", aOutTransR, aOutTransG, aOutTransB, &status);
    CHECK_MSTATUS(status);

    CHECK_MSTATUS(nAttr.setHidden(false));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));

    // Camera Normals
    //
    aNormalCameraX = nAttr.create("normalCameraX", "nx", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aNormalCameraY = nAttr.create("normalCameraY", "ny", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aNormalCameraZ = nAttr.create("normalCameraZ", "nz", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aNormalCamera = nAttr.create("normalCamera", "n", aNormalCameraX, aNormalCameraY, aNormalCameraZ, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f, 1.0f, 1.0f));
    CHECK_MSTATUS(nAttr.setHidden(true));

    // Light Direction
    //
    aLightDirectionX = nAttr.create("lightDirectionX", "ldx", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightDirectionY = nAttr.create("lightDirectionY", "ldy", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightDirectionZ = nAttr.create("lightDirectionZ", "ldz", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightDirection =
        nAttr.create("lightDirection", "ld", aLightDirectionX, aLightDirectionY, aLightDirectionZ, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f, 1.0f, 1.0f));

    // Light Intensity
    //
    aLightIntensityR = nAttr.create("lightIntensityR", "lir", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightIntensityG = nAttr.create("lightIntensityG", "lig", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightIntensityB = nAttr.create("lightIntensityB", "lib", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightIntensity =
        nAttr.create("lightIntensity", "li", aLightIntensityR, aLightIntensityG, aLightIntensityB, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f, 1.0f, 1.0f));

    // Light
    //
    aLightAmbient = nAttr.create("lightAmbient", "la", MFnNumericData::kBoolean, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(true));

    aLightDiffuse = nAttr.create("lightDiffuse", "ldf", MFnNumericData::kBoolean, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(true));

    aLightSpecular = nAttr.create("lightSpecular", "ls", MFnNumericData::kBoolean, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(false));

    aLightShadowFraction = nAttr.create("lightShadowFraction", "lsf", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aPreShadowIntensity = nAttr.create("preShadowIntensity", "psi", MFnNumericData::kFloat, 0, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));
    CHECK_MSTATUS(nAttr.setDefault(1.0f));

    aLightBlindData = nAttr.createAddr("lightBlindData", "lbld", &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(nAttr.setStorable(false));
    CHECK_MSTATUS(nAttr.setHidden(true));
    CHECK_MSTATUS(nAttr.setReadable(true));
    CHECK_MSTATUS(nAttr.setWritable(false));

    aLightData = lAttr.create("lightDataArray", "ltd", aLightDirection, aLightIntensity, aLightAmbient, aLightDiffuse,
        aLightSpecular, aLightShadowFraction, aPreShadowIntensity, aLightBlindData, &status);
    CHECK_MSTATUS(status);
    CHECK_MSTATUS(lAttr.setArray(true));
    CHECK_MSTATUS(lAttr.setStorable(false));
    CHECK_MSTATUS(lAttr.setHidden(true));
    CHECK_MSTATUS(lAttr.setDefault(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, true, false, 1.0f, 1.0f, NULL));

    // Next we will add the attributes we have defined to the node
    //
    CHECK_MSTATUS(addAttribute(aTranslucenceCoeff));
    CHECK_MSTATUS(addAttribute(aDiffuseReflectivity));
    CHECK_MSTATUS(addAttribute(aColor));
    CHECK_MSTATUS(addAttribute(aIncandescence));
    CHECK_MSTATUS(addAttribute(aInTransparency));
    CHECK_MSTATUS(addAttribute(aOutColor));
    CHECK_MSTATUS(addAttribute(aOutTransparency));
    CHECK_MSTATUS(addAttribute(aNormalCamera));

    // Only add the parent of the compound
    CHECK_MSTATUS(addAttribute(aLightData));

    // The attributeAffects() method is used to indicate when the input
    // attribute affects the output attribute. This knowledge allows Maya
    // to optimize dependencies in the graph in more complex nodes where
    // there may be several inputs and outputs, but not all the inputs
    // affect all the outputs.
    //
    CHECK_MSTATUS(attributeAffects(aTranslucenceCoeff, aOutColor));
    CHECK_MSTATUS(attributeAffects(aDiffuseReflectivity, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColorR, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColorG, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColorB, aOutColor));
    CHECK_MSTATUS(attributeAffects(aColor, aOutColor));
    CHECK_MSTATUS(attributeAffects(aInTransR, aOutTransparency));
    CHECK_MSTATUS(attributeAffects(aInTransG, aOutTransparency));
    CHECK_MSTATUS(attributeAffects(aInTransB, aOutTransparency));
    CHECK_MSTATUS(attributeAffects(aInTransparency, aOutTransparency));
    CHECK_MSTATUS(attributeAffects(aInTransparency, aOutColor));
    CHECK_MSTATUS(attributeAffects(aIncandescenceR, aOutColor));
    CHECK_MSTATUS(attributeAffects(aIncandescenceG, aOutColor));
    CHECK_MSTATUS(attributeAffects(aIncandescenceB, aOutColor));
    CHECK_MSTATUS(attributeAffects(aIncandescence, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightIntensityR, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightIntensityB, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightIntensityG, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightIntensity, aOutColor));
    CHECK_MSTATUS(attributeAffects(aNormalCameraX, aOutColor));
    CHECK_MSTATUS(attributeAffects(aNormalCameraY, aOutColor));
    CHECK_MSTATUS(attributeAffects(aNormalCameraZ, aOutColor));
    CHECK_MSTATUS(attributeAffects(aNormalCamera, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightDirectionX, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightDirectionY, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightDirectionZ, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightDirection, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightAmbient, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightSpecular, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightDiffuse, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightShadowFraction, aOutColor));
    CHECK_MSTATUS(attributeAffects(aPreShadowIntensity, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightBlindData, aOutColor));
    CHECK_MSTATUS(attributeAffects(aLightData, aOutColor));

    // loading x-ray part
    xrDoubleSide = nAttr.create("xrayDoubleSide", "xrd", MFnNumericData::kBoolean, 0, &status);
    MAKE_INPUT(nAttr);

    LPSTRVec lst;
    LPSTRIt it;

    xrEngineData = eAttr.create("xrayEngineShader", "xre");
    MAKE_INPUT(eAttr);
    LoadBlenderList(lst);
    for (it = lst.begin(); it != lst.end(); it++)
    {
        _ChangeSymbol(*it, '\\', '/');
        CHECK_MSTATUS(eAttr.addField(*it, crc16_calc((u8*)*it, xr_strlen(*it))));
    }
#if MAYA_API_VERSION > 450
    CHECK_MSTATUS(eAttr.setDefault(eAttr.fieldIndex("default")));
#else
#endif
    ClearList(lst);

    xrCompilerData = eAttr.create("xrayCompilerShader", "xrc");
    MAKE_INPUT(eAttr);
    LoadShaderLCList(lst);
    for (it = lst.begin(); it != lst.end(); it++)
    {
        _ChangeSymbol(*it, '\\', '/');
        CHECK_MSTATUS(eAttr.addField(*it, crc16_calc((u8*)*it, xr_strlen(*it))));
    }
#if MAYA_API_VERSION > 450
    CHECK_MSTATUS(eAttr.setDefault(eAttr.fieldIndex("default")));
#else
#endif
    ClearList(lst);

    xrMaterialData = eAttr.create("xrayGameMaterial", " ");
    MAKE_INPUT(eAttr);
    LoadGameMtlList(lst);
    for (it = lst.begin(); it != lst.end(); it++)
    {
        _ChangeSymbol(*it, '\\', '/');
        CHECK_MSTATUS(eAttr.addField(*it, crc16_calc((u8*)*it, xr_strlen(*it))));
    }
#if MAYA_API_VERSION > 450
    CHECK_MSTATUS(eAttr.setDefault(eAttr.fieldIndex("default")));
#else
#endif
    ClearList(lst);

    CHECK_MSTATUS(addAttribute(xrDoubleSide));
    CHECK_MSTATUS(addAttribute(xrEngineData));
    CHECK_MSTATUS(addAttribute(xrCompilerData));
    CHECK_MSTATUS(addAttribute(xrMaterialData));
    // end x-ray part

    return (MS::kSuccess);
}

// The compute() method does the actual work of the node using the inputs
// of the node to generate its output.
//
// Compute takes two parameters: plug and data.
// - Plug is the the data value that needs to be recomputed
// - Data provides handles to all of the nodes attributes, only these
//   handles should be used when performing computations.
//
MStatus CXRayMtl::compute(const MPlug& plug, MDataBlock& block)
{
    // The plug parameter will allow us to determine which output attribute
    // needs to be calculated.
    //
    if (plug == aOutColor || plug == aOutColorR || plug == aOutColorG || plug == aOutColorB ||
        plug == aOutTransparency || plug == aOutTransR || plug == aOutTransG || plug == aOutTransB)
    {
        MStatus status;
        MFloatVector resultColor(0.0, 0.0, 0.0);

        // Get surface shading parameters from input block
        //
        MFloatVector& surfaceNormal = block.inputValue(aNormalCamera, &status).asFloatVector();
        CHECK_MSTATUS(status);

        MFloatVector& surfaceColor = block.inputValue(aColor, &status).asFloatVector();
        CHECK_MSTATUS(status);

        MFloatVector& incandescence = block.inputValue(aIncandescence, &status).asFloatVector();
        CHECK_MSTATUS(status);

        float diffuseReflectivity = block.inputValue(aDiffuseReflectivity, &status).asFloat();
        CHECK_MSTATUS(status);

        // 		float translucenceCoeff = block.inputValue( aTranslucenceCoeff,
        // 				&status ).asFloat();
        // 		CHECK_MSTATUS( status );

        // Get light list
        //
        MArrayDataHandle lightData = block.inputArrayValue(aLightData, &status);
        CHECK_MSTATUS(status);

        int numLights = lightData.elementCount(&status);
        CHECK_MSTATUS(status);

        // Calculate the effect of the lights in the scene on the color
        //

        // Iterate through light list and get ambient/diffuse values
        //
        for (int count = 1; count <= numLights; count++)
        {
            // Get the current light out of the array
            //
            MDataHandle currentLight = lightData.inputValue(&status);
            CHECK_MSTATUS(status);

            // Get the intensity of that light
            //
            MFloatVector& lightIntensity = currentLight.child(aLightIntensity).asFloatVector();

            // Find ambient component
            //
            if (currentLight.child(aLightAmbient).asBool())
            {
                resultColor += lightIntensity;
            }

            // Find diffuse component
            //
            if (currentLight.child(aLightDiffuse).asBool())
            {
                MFloatVector& lightDirection = currentLight.child(aLightDirection).asFloatVector();
                float cosln = lightDirection * surfaceNormal;

                if (cosln > 0.0f)
                {
                    resultColor += lightIntensity * (cosln * diffuseReflectivity);
                }
            }

            // Advance to the next light.
            //
            if (count < numLights)
            {
                status = lightData.next();
                CHECK_MSTATUS(status);
            }
        }

        // Factor incident light with surface color and add incandescence
        //
        resultColor[0] = resultColor[0] * surfaceColor[0] + incandescence[0];
        resultColor[1] = resultColor[1] * surfaceColor[1] + incandescence[1];
        resultColor[2] = resultColor[2] * surfaceColor[2] + incandescence[2];

        // Set ouput color attribute
        //
        if (plug == aOutColor || plug == aOutColorR || plug == aOutColorG || plug == aOutColorB)
        {
            // Get the handle to the attribute
            //
            MDataHandle outColorHandle = block.outputValue(aOutColor, &status);
            CHECK_MSTATUS(status);
            MFloatVector& outColor = outColorHandle.asFloatVector();

            outColor = resultColor; // Set the output value
            outColorHandle.setClean(); // Mark the output value as clean
        }

        // Set ouput transparency
        //
        if (plug == aOutTransparency || plug == aOutTransR || plug == aOutTransG || plug == aOutTransB)
        {
            MFloatVector& transparency = block.inputValue(aInTransparency, &status).asFloatVector();
            CHECK_MSTATUS(status);

            // Get the handle to the attribute
            //
            MDataHandle outTransHandle = block.outputValue(aOutTransparency, &status);
            CHECK_MSTATUS(status);
            MFloatVector& outTrans = outTransHandle.asFloatVector();

            outTrans = transparency; // Set the output value
            outTransHandle.setClean(); // Mark the output value as clean
        }
    }
    else
    {
        return (MS::kUnknownParameter); // We got an unexpected plug
    }

    return (MS::kSuccess);
}

MStatus uninitializePlugin(MObject obj)
{
    const MString UserClassify("shader/surface");

    MString command("if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"");

    MFnPlugin plugin(obj);

    CHECK_MSTATUS(plugin.deregisterNode(CXRayMtl::id));

    command += UserClassify;
    command += "\");}\n";

    CHECK_MSTATUS(MGlobal::executeCommand(command));

    Core._destroy();

    return MS::kSuccess;
}

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

// These methods load and unload the plugin, registerNode registers the
// new node type with maya
//
MStatus initializePlugin(MObject obj)
{
    INIT_OBJ = obj;
    const MString UserClassify("shader/surface");

    Core.Initialize("XRayMayaPlugin", nullptr, LogCallback(ELogCallback, nullptr), FALSE, nullptr, true);
    FS._initialize(CLocatorAPI::flScanAppRoot, NULL, "xray_path.ltx");

    MString command("if( `window -exists createRenderNodeWindow` ) {refreshCreateRenderNodeWindow(\"");

    MFnPlugin plugin(obj, "GSC Game World", "1.00", "Any");

    CHECK_MSTATUS(plugin.registerNode(
        "XRayMtl", CXRayMtl::id, CXRayMtl::creator, CXRayMtl::initialize, MPxNode::kDependNode, &UserClassify));

    command += UserClassify;

    command += "\");}\n";

    CHECK_MSTATUS(MGlobal::executeCommand(command));

    // add callbacks
    //	MSceneMessage::addCallback(MSceneMessage::kMayaExiting,  uninitialize, 0);

    return (MS::kSuccess);
}
