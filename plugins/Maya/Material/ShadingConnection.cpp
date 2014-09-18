///////////////////////////////////////////////////////////////////
// DESCRIPTION:  This class stores useful information about
//				 a shader's attribute, including what's connected upstream of it.
//				 It also automatically passes through shader switches.
//
// AUTHOR: Christian Laforte
//
///////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "ShadingConnection.h"

ShadingConnection::ShadingConnection(MObject shaderObj, MString shapeName, MString attribute /* = "" */)
{
	// Store those input values for later use.
	this->m_shaderObj = shaderObj;
	this->m_shapeName = shapeName;

	// By default, the connection is direct until proven otherwise.
	this->m_directConnection = true;

	if (attribute != "")
		traverseAttribute(attribute);
}

ShadingConnection::TYPE ShadingConnection::traverseAttribute(MString attributeName)
{
	// Get a plug to the attribute.
	MStatus status;
	MFnDependencyNode shaderNode(m_shaderObj);

	MPlug plug = shaderNode.findPlug( attributeName, &status );
	assert(status);

	this->m_attributeName = attributeName;

	return analyzePlug(plug);
}
	
ShadingConnection::TYPE ShadingConnection::analyzePlug(MPlug plug)
{
	MStatus status;

	// Find all incoming connections.
	MPlugArray connectedElements;
	plug.connectedTo( connectedElements, true, false, &status );
	assert(status);
	
	if (connectedElements.length() == 0)
	{
		// It's a constant color... set it now and return.
		unsigned int numChildren = plug.numChildren();
		assert(numChildren == 3);

		float red, green, blue;
		plug.child(0).getValue(red);
		plug.child(1).getValue(green);
		plug.child(2).getValue(blue);

		return setConstantColor(MColor(red, green, blue, 1.0));
	}
	
	// The plug is actually connected on a different shading node.
	MObject connectedObject = connectedElements[0].node(&status);
	assert(status);

	// If this is a triple shading switch, go through it.
	if (connectedObject.hasFn(MFn::kTripleShadingSwitch))
	{
		return traverseTripleShadingSwitch(connectedObject);
	}

	// Set the object and return the appropriate type.
	return setTexture(connectedObject);
}

ShadingConnection::TYPE ShadingConnection::traverseTripleShadingSwitch(MObject connectedObject)
{			
	// The connection cannot be direct anymore.
	m_directConnection = false;
	
	MFnDependencyNode node(connectedObject);

	MStatus status;
	MPlug inputPlug = node.findPlug( "input" );
	
	unsigned int numElements = inputPlug.numElements();
	unsigned int numChildren = inputPlug.numChildren();
	
	// It is assumed that the first child of this input compound attribute is
	// a "inTexture" attribute and the the second child is the inShape attribute.
	// This assumption should always be safe, unless some drastic changes occured in the
	// implementation of the shading switches... in which case this code won't work, so
	// we would throw an exception.
	assert( numChildren >= 2); // "Abnormal hierarchy in switching node."
	
	// Go through each row of the switch table.
	for (unsigned int index = 0; index < numElements; index++)
	{
		MPlug inTexturePlug = inputPlug.elementByPhysicalIndex(index).child(0);
		MPlug inShapePlug = inputPlug.elementByPhysicalIndex(index).child(1);				

		MPlugArray inShapeConnections;
		inShapePlug.connectedTo( inShapeConnections, true, false, &status );
		
		// Check if one of the corresponding shapes matches the given shape name.
		unsigned int numCorrespondingShapes = inShapeConnections.length();
		for (unsigned int shapeNum = 0; shapeNum < numCorrespondingShapes; shapeNum++)
		{
			MObject inShapeObj = inShapeConnections[0].node(&status);
			assert(status);
			
			MFnDependencyNode inShape(inShapeObj);
			MString inShapeName = inShape.name();

			if (inShapeName == m_shapeName)
				return analyzePlug(inTexturePlug);
		}
	}

	// If there was no special case in the switch for our shape, check the default.
	MPlug defaultPlug = node.findPlug("default");
	return analyzePlug(defaultPlug);
}

ShadingConnection::TYPE ShadingConnection::type()
{
	return m_type;
}

MColor ShadingConnection::constantColor()
{
	assert(m_type == CONSTANT_COLOR);
	return m_constantColor;
}

MObject ShadingConnection::texture()
{
	assert(m_type == TEXTURE);
	return m_texture;
}

MObject ShadingConnection::shaderObj()
{
	return m_shaderObj;
}

MString ShadingConnection::shaderName()
{
	MStatus stat;
	MFnDependencyNode shaderNode(m_shaderObj, &stat);
	assert(stat);
	return shaderNode.name();
}

MString ShadingConnection::attributeName()
{
	return m_attributeName;
}

MString ShadingConnection::shapeName()
{
	return m_shapeName;
}

ShadingConnection::TYPE ShadingConnection::setConstantColor(MColor col)
{
	m_constantColor = col;
	m_type = CONSTANT_COLOR;

	// Reset m_texture to prevent misinterpretation.
	m_texture = MObject();

	return m_type;
}

ShadingConnection::TYPE ShadingConnection::setTexture(MObject texture)
{
	m_texture = texture;
	m_type = TEXTURE;

	// Reset m_color to prevent misinterpretation.
	m_constantColor = MColor();

	return m_type;
}

bool ShadingConnection::isDirectConnection()
{
	return m_directConnection;
}
