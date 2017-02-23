#include "stdafx.h"

#include "ExportDefines.h"

MObject findShader(MObject& setNode, SXRShaderData& d)
//
//  Description:
//      Find the shading node for the given shading group set node.
//
{
    MFnDependencyNode fnNode(setNode);
    d.name = fnNode.name();
    // cout << "looking for shader in node " << fnNode.name().asChar() << "\n";
    MPlug shaderPlug = fnNode.findPlug("surfaceShader");

    if (!shaderPlug.isNull())
    {
        MPlugArray connectedPlugs;
        bool asSrc = false;
        bool asDst = true;
        shaderPlug.connectedTo(connectedPlugs, asDst, asSrc);

        if (connectedPlugs.length() != 1)
            Msg("!Error getting shader");
        else
            return connectedPlugs[0].node();
    }

    Msg("!Error finding surface shader for node '%s'", fnNode.name().asChar());
    return MObject::kNullObj;
}

MStatus parseShader(MObject& src, SXRShaderData& d)
{
    MStatus status;

    MFnSet fnSet(src, &status);
    if (status == MStatus::kFailure)
    {
        status.perror("Unable to lookup shader from set of shaders for object");
        return status;
    }

    MObject shaderNode = findShader(src, d);
    if (shaderNode == MObject::kNullObj)
        return (MStatus::kFailure);

    MPlug colorPlug = MFnDependencyNode(shaderNode).findPlug("color", &status);
    if (status == MStatus::kFailure)
        return (status);

    MItDependencyGraph dgIt(colorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream,
        MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);

    if (status == MStatus::kFailure)
        return (status);

    dgIt.disablePruningOnFilter();

    // If no texture file node was found, just continue.
    //
    if (dgIt.isDone())
    {
        //		cout << "no textures found for " << colorPlug.name() << "\n";
        return (MStatus::kSuccess);
    }

    // Print out the texture node name and texture file that it references.
    //
    MObject textureNode = dgIt.thisNode();
    MPlug filenamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
    MString textureName;
    filenamePlug.getValue(textureName);

    MStringArray rgFolders;

    if (strchr(textureName.asChar(), '\\'))
    {
        textureName.split('\\', rgFolders);
    }
    else
    {
        textureName.split('/', rgFolders);
    }
    d.tex_name = rgFolders[rgFolders.length() - 1];
    //	cout << "Found texture file: '" << filename.asChar() << "'\n";

    short index;
    // double side flag
    MPlug xrDoubleSidePlug = MFnDependencyNode(shaderNode).findPlug("xrayDoubleSide", &status);
    if (status == MS::kSuccess)
    {
        MFnEnumAttribute enm = xrDoubleSidePlug.attribute();
        if ((status == MS::kSuccess) && (MS::kSuccess == xrDoubleSidePlug.getValue(index)))
            d.double_side = index;
    }
    // engine
    MPlug xrEnginePlug = MFnDependencyNode(shaderNode).findPlug("xrayEngineShader", &status);
    if (status == MS::kSuccess)
    {
        MFnEnumAttribute enm = xrEnginePlug.attribute();

        if ((status == MS::kSuccess) && (MS::kSuccess == xrEnginePlug.getValue(index)))
            d.eng_name = enm.fieldName(index);
    }
    // compiler
    MPlug xrCompilerPlug = MFnDependencyNode(shaderNode).findPlug("xrayCompilerShader", &status);
    if (status == MS::kSuccess)
    {
        MFnEnumAttribute enm = xrCompilerPlug.attribute();
        if ((status == MS::kSuccess) && (MS::kSuccess == xrCompilerPlug.getValue(index)))
            d.comp_name = enm.fieldName(index);
    }
    // game material
    MPlug xrGameMaterialPlug = MFnDependencyNode(shaderNode).findPlug("xrayGameMaterial", &status);
    if (status == MS::kSuccess)
    {
        MFnEnumAttribute enm = xrGameMaterialPlug.attribute();
        if ((status == MS::kSuccess) && (MS::kSuccess == xrGameMaterialPlug.getValue(index)))
            d.gmat_name = enm.fieldName(index);
    }

    return (MStatus::kSuccess);
}
