#ifndef MAYA_ShadingConnection
#define MAYA_ShadingConnection

///////////////////////////////////////////////////////////////////
// DESCRIPTION:  This class stores useful information about
//				 a shader's attribute, including what's connected upstream of it.
//				 It also automatically passes through shader switches.
//
// The ShadingConnection is a helper class that stores useful information 
// about a shader's attribute, including what's connected upstream of it.
//
// It can:
//		Traverse upstream until it finds if the connection is ultimately 
// on a constant color, or a shading object. This process goes through 
// shading switches if it encounters some. The object/color found is called
// the "target".
//
// At the end of this process, the following information is stored:
// 1. Target type: whether or not the connection ends up on a shading 
//				   object (for example: file texture), or a constant 
//				   color (ex: bright red).
// 2. Target identity: either the color, or the target object itself. Note
//					   that the caller is responsible to first check the type
//					   before calling color() or object().
// 3. Directness: Whether or not the connection was direct (ie: there was
//				  no shading switch in between the source and the target)
//			      or indirect (ie: there were shading switches in between.)
//
// In order to traverse the shading network, we need the name of the shape.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////

#include <maya/MColor.h>

class ShadingConnection
{
public:
	enum TYPE
	{
		CONSTANT_COLOR, 
		TEXTURE
	};

	ShadingConnection(MObject shaderObj, MString shapeName, MString attribute = "");

	TYPE traverseAttribute(MString attributeName);
	
	TYPE analyzePlug(MPlug plug);
	
	TYPE traverseTripleShadingSwitch(MObject connectedObject);

	TYPE type();

	MColor constantColor();

	MObject texture();

	MObject shaderObj();

	MString shaderName();

	MString attributeName();

	MString shapeName();

	TYPE setConstantColor(MColor col);
	TYPE setTexture(MObject texture);

	bool isDirectConnection();

	
private:
	// Input
	MObject m_shaderObj;
	MString m_attributeName;
	MString m_shapeName;

	// Output	
	TYPE m_type;
	MColor m_constantColor;
	MObject m_texture;
	bool m_directConnection;


};


#endif // MAYA_ShadingConnection