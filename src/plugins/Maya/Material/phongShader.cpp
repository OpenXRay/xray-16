#include "stdafx.h"
using namespace std;

#include <maya/MPxNode.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MPlug.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnLightDataAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnPlugin.h>

// add for raytracing api enhancement
#include <maya/MRenderUtil.h>
#include <maya/MDrawRequest.h>

#include "BlenderListLoader.h"
//
// DESCRIPTION:
///////////////////////////////////////////////////////
class CXRayMtl : public MPxNode
{
	public:
						CXRayMtl();
    virtual				~CXRayMtl();

    virtual MStatus		compute( const MPlug&, MDataBlock& );

	virtual void      postConstructor();

    static void *     creator();
    static MStatus    initialize();
    static MTypeId    id;

	private:
	static MObject aColor;
    static MObject aTranslucenceCoeff;
    static MObject aDiffuseReflectivity;
	static MObject aIncandescence;
	static MObject aPointCamera;
	static MObject aNormalCamera;
	static MObject aLightDirection;
	static MObject aLightIntensity;
    static MObject aPower;
    static MObject aSpecularity;
    static MObject aLightAmbient;
    static MObject aLightDiffuse;
    static MObject aLightSpecular;
    static MObject aLightShadowFraction;
    static MObject aPreShadowIntensity;
    static MObject aLightBlindData;
    static MObject aLightData;
	
	static MObject aXREngineData;
	static MObject aXRCompilerData;
	static MObject aXRMaterialData;

    static MObject aRayOrigin;
    static MObject aRayDirection;

    static MObject aObjectId;
    static MObject aRaySampler;
    static MObject aRayDepth;

    static MObject aReflectGain;

	static MObject aTriangleNormalCamera;

	static MObject aOutColor;

};

// Static data
MTypeId CXRayMtl::id( 0x81001 );

// Attributes 
MObject CXRayMtl::aColor;
MObject CXRayMtl::aTranslucenceCoeff;
MObject CXRayMtl::aDiffuseReflectivity;
MObject CXRayMtl::aIncandescence;
MObject CXRayMtl::aOutColor;
MObject CXRayMtl::aPointCamera;
MObject CXRayMtl::aNormalCamera;
MObject CXRayMtl::aLightData;

MObject CXRayMtl::aXREngineData;
MObject CXRayMtl::aXRCompilerData;
MObject CXRayMtl::aXRMaterialData;

MObject CXRayMtl::aLightDirection;
MObject CXRayMtl::aLightIntensity;
MObject CXRayMtl::aLightAmbient;
MObject CXRayMtl::aLightDiffuse;
MObject CXRayMtl::aLightSpecular;
MObject CXRayMtl::aLightShadowFraction;
MObject CXRayMtl::aPreShadowIntensity;
MObject CXRayMtl::aLightBlindData;
MObject CXRayMtl::aPower;
MObject CXRayMtl::aSpecularity;

MObject CXRayMtl::aRayOrigin;
MObject CXRayMtl::aRayDirection;
MObject CXRayMtl::aObjectId;
MObject CXRayMtl::aRaySampler;
MObject CXRayMtl::aRayDepth;

MObject CXRayMtl::aReflectGain;

MObject CXRayMtl::aTriangleNormalCamera;

#define MAKE_INPUT(attr)						\
    CHECK_MSTATUS ( attr.setKeyable(true) );  	\
	CHECK_MSTATUS ( attr.setStorable(true) );	\
    CHECK_MSTATUS ( attr.setReadable(true) );  \
	CHECK_MSTATUS ( attr.setWritable(true) );

#define MAKE_OUTPUT(attr)							\
    CHECK_MSTATUS ( attr.setKeyable(false) ) ;  	\
	CHECK_MSTATUS ( attr.setStorable(false) );		\
    CHECK_MSTATUS ( attr.setReadable(true) ) ;  	\
	CHECK_MSTATUS ( attr.setWritable(false) );

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void CXRayMtl::postConstructor( )
{
	setMPSafe(true);
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
CXRayMtl::CXRayMtl()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
CXRayMtl::~CXRayMtl()
{
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
void * CXRayMtl::creator()
{
	Core._initialize("XRayPlugin",ELogCallback,"\\\\X-Ray\\stalker$\\");
	
	return new CXRayMtl();
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus CXRayMtl::initialize()
{
    MFnNumericAttribute nAttr; 
    MFnLightDataAttribute lAttr;
	MFnEnumAttribute eAttr;
/*
	// loading x-ray part
	LPSTRVec lst;
	LPSTRIt it;
	int k=0;
	aXREngineData = eAttr.create( "engineShader", "xre");
	MAKE_INPUT(eAttr);
	LoadBlenderList(lst);
	for (it=lst.begin(); it!=lst.end(); it++){
		_ChangeSymbol(*it,'\\','/');
		CHECK_MSTATUS (eAttr.addField(*it,it-lst.begin()));
	}
	ClearList(lst);

	aXRCompilerData = eAttr.create( "compilerShader", "xrc");
	MAKE_INPUT(eAttr);
	LoadShaderLCList(lst);
	for (it=lst.begin(); it!=lst.end(); it++){
		_ChangeSymbol(*it,'\\','/');
		CHECK_MSTATUS (eAttr.addField(*it,it-lst.begin()));
	}
	ClearList(lst);

	aXRMaterialData = eAttr.create( "gameMaterial", "xrm");
	MAKE_INPUT(eAttr);
	LoadGameMtlList(lst);
	for (it=lst.begin(); it!=lst.end(); it++){
		_ChangeSymbol(*it,'\\','/');
		CHECK_MSTATUS (eAttr.addField(*it,it-lst.begin()));
	}
	ClearList(lst);
	// end x-ray part
*/
	aTranslucenceCoeff = nAttr.create("translucenceCoeff", "tc", 
									  MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);

    aDiffuseReflectivity = nAttr.create("diffuseReflectivity", "drfl",
										MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(0.8f) );

    aColor = nAttr.createColor( "color", "c" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(0.0f, 0.58824f, 0.644f) );

    aIncandescence = nAttr.createColor( "incandescence", "ic" );
    MAKE_INPUT(nAttr);

    aOutColor = nAttr.createColor( "outColor", "oc" );
    MAKE_OUTPUT(nAttr);

    aPointCamera = nAttr.createPoint( "pointCamera", "pc" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 1.0f, 1.0f) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aPower = nAttr.create( "power", "pow", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setMin(0.0f) );
    CHECK_MSTATUS ( nAttr.setMax(200.0f) );
    CHECK_MSTATUS ( nAttr.setDefault(10.0f) );

    aSpecularity = nAttr.create( "specularity", "spc", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setMin(0.0f) );
    CHECK_MSTATUS ( nAttr.setMax(1.0f) ) ;
    CHECK_MSTATUS ( nAttr.setDefault(0.5f) );

    aReflectGain = nAttr.create( "reflectionGain", "rg", MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setMin(0.0f) );
    CHECK_MSTATUS ( nAttr.setMax(1.0f) );
    CHECK_MSTATUS ( nAttr.setDefault(0.5f) );

    aNormalCamera = nAttr.createPoint( "normalCamera", "n" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 1.0f, 1.0f) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aTriangleNormalCamera = nAttr.createPoint( "triangleNormalCamera", "tn" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 1.0f, 1.0f));
    CHECK_MSTATUS ( nAttr.setHidden(true));

    aLightDirection = nAttr.createPoint( "lightDirection", "ld" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 1.0f, 1.0f) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightIntensity = nAttr.createColor( "lightIntensity", "li" );
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setDefault(1.0f, 1.0f, 1.0f) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightAmbient = nAttr.create( "lightAmbient", "la",
								  MFnNumericData::kBoolean);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightDiffuse = nAttr.create( "lightDiffuse", "ldf", 
								  MFnNumericData::kBoolean);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightSpecular = nAttr.create( "lightSpecular", "ls", 
								   MFnNumericData::kBoolean);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightShadowFraction = nAttr.create("lightShadowFraction", "lsf",
										MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aPreShadowIntensity = nAttr.create("preShadowIntensity", "psi",
									   MFnNumericData::kFloat);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightBlindData = nAttr.create("lightBlindData", "lbld", 
								   MFnNumericData::kLong);
    MAKE_INPUT(nAttr);
    CHECK_MSTATUS ( nAttr.setHidden(true) );

    aLightData = lAttr.create( "lightDataArray", "ltd", 
                               aLightDirection, aLightIntensity, aLightAmbient,
                               aLightDiffuse, aLightSpecular, 
							   aLightShadowFraction,
                               aPreShadowIntensity,
                               aLightBlindData);
    CHECK_MSTATUS ( lAttr.setArray(true) );
    CHECK_MSTATUS ( lAttr.setStorable(false) );
    CHECK_MSTATUS ( lAttr.setHidden(true) );
    CHECK_MSTATUS ( lAttr.setDefault(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true, true,
					 false, 0.0f, 1.0f, 0) );

	// rayOrigin
	MObject RayX = nAttr.create( "rayOx", "rxo", MFnNumericData::kFloat, 0.0 );
	MObject RayY = nAttr.create( "rayOy", "ryo", MFnNumericData::kFloat, 0.0 );
	MObject RayZ = nAttr.create( "rayOz", "rzo", MFnNumericData::kFloat, 0.0 );
	aRayOrigin = nAttr.create( "rayOrigin", "rog", RayX, RayY, RayZ );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(false) );

	// rayDirection 
	RayX = nAttr.create( "rayDirectionX", "rdx", MFnNumericData::kFloat, 1.0 );
	RayY = nAttr.create( "rayDirectionY", "rdy", MFnNumericData::kFloat, 0.0 );
	RayZ = nAttr.create( "rayDirectionZ", "rdz", MFnNumericData::kFloat, 0.0 );
	aRayDirection = nAttr.create( "rayDirection", "rad", RayX, RayY, RayZ );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(false) );

	// objectId
	aObjectId = nAttr.create( "objectId", "oi", MFnNumericData::kLong, 0.0 );
    CHECK_MSTATUS ( nAttr.setStorable(false) ); 
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(false) );

	// raySampler
	aRaySampler = nAttr.create("raySampler", "rtr", MFnNumericData::kLong,0.0);
    CHECK_MSTATUS ( nAttr.setStorable(false));
    CHECK_MSTATUS ( nAttr.setHidden(true) );
    CHECK_MSTATUS ( nAttr.setReadable(false) );

	// rayDepth
	aRayDepth = nAttr.create( "rayDepth", "rd", MFnNumericData::kShort, 0.0 );
    CHECK_MSTATUS ( nAttr.setStorable(false) );
    CHECK_MSTATUS  (nAttr.setHidden(true) ) ;
    CHECK_MSTATUS ( nAttr.setReadable(false) );

	CHECK_MSTATUS ( addAttribute(aXREngineData) );
	CHECK_MSTATUS ( addAttribute(aXRCompilerData) );
	CHECK_MSTATUS ( addAttribute(aXRMaterialData) );

	CHECK_MSTATUS ( addAttribute(aTranslucenceCoeff) );
    CHECK_MSTATUS ( addAttribute(aDiffuseReflectivity) );
    CHECK_MSTATUS ( addAttribute(aColor) );
    CHECK_MSTATUS ( addAttribute(aIncandescence) );
    CHECK_MSTATUS ( addAttribute(aPointCamera) );
    CHECK_MSTATUS ( addAttribute(aNormalCamera) );
    CHECK_MSTATUS ( addAttribute(aTriangleNormalCamera) );

    CHECK_MSTATUS ( addAttribute(aLightData) );

    CHECK_MSTATUS ( addAttribute(aPower) );
    CHECK_MSTATUS ( addAttribute(aSpecularity) );
    CHECK_MSTATUS ( addAttribute(aOutColor) );

	CHECK_MSTATUS ( addAttribute(aRayOrigin) );
	CHECK_MSTATUS ( addAttribute(aRayDirection) );
	CHECK_MSTATUS ( addAttribute(aObjectId) );
	CHECK_MSTATUS ( addAttribute(aRaySampler) );
	CHECK_MSTATUS ( addAttribute(aRayDepth) );
    CHECK_MSTATUS ( addAttribute(aReflectGain) );

	CHECK_MSTATUS ( attributeAffects (aXREngineData, aXREngineData));
	CHECK_MSTATUS ( attributeAffects (aXRCompilerData, aXRCompilerData));
	CHECK_MSTATUS ( attributeAffects (aXRMaterialData, aXRMaterialData));

	CHECK_MSTATUS ( attributeAffects (aTranslucenceCoeff, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aDiffuseReflectivity, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightIntensity, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aIncandescence, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aPointCamera, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aNormalCamera, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aTriangleNormalCamera, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightData, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightAmbient, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightSpecular, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightDiffuse, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightDirection, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightShadowFraction, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aPreShadowIntensity, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aLightBlindData, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aPower, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aSpecularity, aOutColor));
    CHECK_MSTATUS ( attributeAffects (aColor, aOutColor));

	CHECK_MSTATUS ( attributeAffects (aRayOrigin,aOutColor));
	CHECK_MSTATUS ( attributeAffects (aRayDirection,aOutColor));
	CHECK_MSTATUS ( attributeAffects (aObjectId,aOutColor));
	CHECK_MSTATUS ( attributeAffects (aRaySampler,aOutColor));
	CHECK_MSTATUS ( attributeAffects (aRayDepth,aOutColor));
    CHECK_MSTATUS ( attributeAffects (aReflectGain,aOutColor) );

    return MS::kSuccess;
}


//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus CXRayMtl::compute(
const MPlug&      plug,
      MDataBlock& block ) 
{ 
    if ((plug != aOutColor) && (plug.parent() != aOutColor))
		return MS::kUnknownParameter;

    MFloatVector resultColor(0.0,0.0,0.0);

    // get sample surface shading parameters
    MFloatVector& surfaceNormal = block.inputValue( aNormalCamera ).asFloatVector();
    MFloatVector& cameraPosition = block.inputValue( aPointCamera ).asFloatVector();

	// use for raytracing api enhancement below
	MFloatVector point = cameraPosition;
	MFloatVector normal = surfaceNormal;

    MFloatVector& surfaceColor  = block.inputValue( aColor ).asFloatVector();
    MFloatVector& incandescence = block.inputValue( aIncandescence ).asFloatVector();
    float diffuseReflectivity = block.inputValue( aDiffuseReflectivity ).asFloat();
    // float translucenceCoeff   = block.inputValue( aTranslucenceCoeff ).asFloat();
	// User-defined Reflection Color Gain
	float reflectGain = block.inputValue( aReflectGain ).asFloat();
  
    // Phong shading attributes
    float power = block.inputValue( aPower ).asFloat();
    float spec = block.inputValue( aSpecularity ).asFloat();

    float specularR, specularG, specularB;
    float diffuseR, diffuseG, diffuseB;
    diffuseR = diffuseG = diffuseB = specularR = specularG = specularB = 0.0;

    // get light list
    MArrayDataHandle lightData = block.inputArrayValue( aLightData );
    int numLights = lightData.elementCount();
    
    // iterate through light list and get ambient/diffuse values
    for( int count=1; count <= numLights; count++ )
    {
        MDataHandle currentLight = lightData.inputValue();
        MFloatVector& lightIntensity = currentLight.child(aLightIntensity).asFloatVector();
        
        // Find the blind data
        int& blindData = currentLight.child( aLightBlindData ).asInt();
     
        // find ambient component
        if( currentLight.child(aLightAmbient).asBool() ) {
            diffuseR += lightIntensity[0];
            diffuseG += lightIntensity[1];
            diffuseB += lightIntensity[2];
        }

        MFloatVector& lightDirection = currentLight.child(aLightDirection).asFloatVector();
        
        if ( blindData == 0 )
        {
			// find diffuse and specular component
			if( currentLight.child(aLightDiffuse).asBool() )
			{			
			    float cosln = lightDirection * surfaceNormal;;				
			    if( cosln > 0.0f )  // calculate only if facing light
			    {
			         diffuseR += lightIntensity[0] * ( cosln * diffuseReflectivity );
			         diffuseG += lightIntensity[1] * ( cosln * diffuseReflectivity );
			         diffuseB += lightIntensity[2] * ( cosln * diffuseReflectivity );
			    }

			    CHECK_MSTATUS( cameraPosition.normalize() );
			    			
				if( cosln > 0.0f ) // calculate only if facing light
				{				
				    float RV = ( ( (2*surfaceNormal) * cosln ) - lightDirection ) * cameraPosition;
				    if( RV > 0.0 ) RV = 0.0;
				    if( RV < 0.0 ) RV = -RV;

				    if ( power < 0 ) power = -power;

				    float s = spec * powf( RV, power );

				    specularR += lightIntensity[0] * s; 
				    specularG += lightIntensity[1] * s; 
				    specularB += lightIntensity[2] * s; 
				}
			}    
        }
        else
        {
			float cosln = MRenderUtil::diffuseReflectance( blindData, lightDirection, point, surfaceNormal, true );
			if( cosln > 0.0f )  // calculate only if facing light
			{
			     diffuseR += lightIntensity[0] * ( cosln * diffuseReflectivity );
			     diffuseG += lightIntensity[1] * ( cosln * diffuseReflectivity );
			     diffuseB += lightIntensity[2] * ( cosln * diffuseReflectivity );
			}

			CHECK_MSTATUS ( cameraPosition.normalize() );
			
			if ( currentLight.child(aLightSpecular).asBool() )
			{
				MFloatVector specLightDirection = lightDirection;
				MDataHandle directionH = block.inputValue( aRayDirection );
				MFloatVector direction = directionH.asFloatVector();
				float lightAttenuation = 1.0;
						
				specLightDirection = MRenderUtil::maximumSpecularReflection( blindData,
										lightDirection, point, surfaceNormal, direction );
				lightAttenuation = MRenderUtil::lightAttenuation( blindData, point, surfaceNormal, false );		

				// Are we facing the light
				if ( specLightDirection * surfaceNormal > 0.0f )
				{			
					float power = block.inputValue( aPower ).asFloat();
					MFloatVector rv = 2 * surfaceNormal * ( surfaceNormal * direction ) - direction;
					float s = spec * powf( rv * specLightDirection, power );
						
					specularR += lightIntensity[0] * s * lightAttenuation; 
					specularG += lightIntensity[1] * s * lightAttenuation; 
					specularB += lightIntensity[2] * s * lightAttenuation;
				}
			 }
       }
       if( !lightData.next() ) break;
    }

    // factor incident light with surface color and add incandescence
    resultColor[0] = ( diffuseR * surfaceColor[0] ) + specularR + incandescence[0];
    resultColor[1] = ( diffuseG * surfaceColor[1] ) + specularG + incandescence[1];
    resultColor[2] = ( diffuseB * surfaceColor[2] ) + specularB + incandescence[2];

	// add the reflection color
	if (reflectGain > 0.0) {

		MStatus status;

		// required attributes for using raytracer
		// origin, direction, sampler, depth, and object id.
		//
		MDataHandle originH = block.inputValue( aRayOrigin, &status);
		MFloatVector origin = originH.asFloatVector();

		MDataHandle directionH = block.inputValue( aRayDirection, &status);
		MFloatVector direction = directionH.asFloatVector();

		MDataHandle samplerH = block.inputValue( aRaySampler, &status);
		int samplerPtr = samplerH.asLong();

		MDataHandle depthH = block.inputValue( aRayDepth, &status);
		short depth = depthH.asShort();

		MDataHandle objH = block.inputValue( aObjectId, &status);
		int objId = objH.asLong();

		MFloatVector reflectColor;
		MFloatVector reflectTransparency;

		MFloatVector& triangleNormal = block.inputValue( aTriangleNormalCamera ).asFloatVector();

		// compute reflected ray
		MFloatVector l = -direction;
		float dot = l * normal;
		if( dot < 0.0 ) dot = -dot;
		MFloatVector refVector = 2 * normal * dot - l; 	// reflection ray
		float dotRef = refVector * triangleNormal;
		if( dotRef < 0.0 ) {
		    const float s = 0.01f;
			MFloatVector mVec = refVector - dotRef * triangleNormal;
			mVec.normalize();
			refVector = mVec + s * triangleNormal;
		}
		CHECK_MSTATUS ( refVector.normalize() );

		status = MRenderUtil::raytrace(
				point,    	//  origin
				refVector,  //  direction
				objId,		//  object id
				samplerPtr, //  sampler info
				depth,		//  ray depth
				reflectColor,	// output color and transp
				reflectTransparency);

		// add in the reflection color
		resultColor[0] += reflectGain * (reflectColor[0]);
		resultColor[1] += reflectGain * (reflectColor[1]);
		resultColor[2] += reflectGain * (reflectColor[2]);
	}

    // set ouput color attribute
    MDataHandle outColorHandle = block.outputValue( aOutColor );
    MFloatVector& outColor = outColorHandle.asFloatVector();
    outColor = resultColor;
    outColorHandle.setClean();

    return MS::kSuccess;
}
//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
   const MString UserClassify( "shader/surface" );

   MFnPlugin plugin( obj, "Alias|Wavefront - Example", "5.0", "Any");
   CHECK_MSTATUS ( plugin.registerNode( "XRayMtl", CXRayMtl::id, 
                         CXRayMtl::creator, CXRayMtl::initialize,
                         MPxNode::kDependNode, &UserClassify ) );

   return MS::kSuccess;
}

//
// DESCRIPTION:
///////////////////////////////////////////////////////
MStatus uninitializePlugin( MObject obj )
{
   MFnPlugin plugin( obj );
   CHECK_MSTATUS ( plugin.deregisterNode( CXRayMtl::id ) );

   return MS::kSuccess;
}
