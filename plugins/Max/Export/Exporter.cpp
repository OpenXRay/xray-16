#include "stdafx.h"
#pragma hdrstop

#include "Exporter.h"
#include "MeshExpUtility.h"

// ================================================== FindPhysiqueModifier()
// Find if a given node contains a Physique Modifier
// DerivedObjectPtr requires you include "modstack.h" from the MAX SDK
Modifier* FindPhysiqueModifier (INode* nodePtr)
{
	// Get object from node. Abort if no object.
	Object* ObjectPtr = nodePtr->GetObjectRef();

	if ( NULL == ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID){
		// Yes -> Cast.
		IDerivedObject* DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers()){
			// Get current modifier.
			Modifier* ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);
			Class_ID clsid = ModifierPtr->ClassID();
			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B))
			{
				// Yes -> Exit.
				return ModifierPtr;
			}

			// Next modifier stack entry.
			ModStackIndex++;
		}
	}

	// Not found.
	return NULL;
}

// ================================================== GetTriObjectFromObjRef
// Return a pointer to a TriObject given an object reference or return NULL
// if the node cannot be converted to a TriObject
TriObject *GetTriObjectFromObjRef(Object* pObj, BOOL *pbDeleteIt) 
{
	TriObject *pTri;

	R_ASSERT(pObj);
	R_ASSERT(pbDeleteIt);

	*pbDeleteIt = FALSE;

	if (pObj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0))){ 
		pTri = (TriObject *) pObj->ConvertToType(0, Class_ID(TRIOBJ_CLASS_ID, 0));

		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (pObj != pTri) 
			*pbDeleteIt = TRUE;

		return pTri;
	}else{
		return NULL;
	}
}

// ================================================== IsExportableMesh()
BOOL IsExportableMesh(INode* pNode, Object* &pObj)
{
	ULONG superClassID;

	R_ASSERT(pNode);
	pObj = pNode->GetObjectRef();

	if (pObj == NULL) return FALSE;

	superClassID = pObj->SuperClassID();
	//find out if mesh is renderable

	if( !pNode->Renderable() || pNode->IsNodeHidden()) return FALSE;

	BOOL bFoundGeomObject = FALSE;
	//find out if mesh is renderable (more)
	switch(superClassID){
	case GEN_DERIVOB_CLASS_ID:
		do{
			pObj = ((IDerivedObject*)pObj)->GetObjRef();
			superClassID = pObj->SuperClassID();
		}
		while( superClassID == GEN_DERIVOB_CLASS_ID );
		switch(superClassID){
		case GEOMOBJECT_CLASS_ID:
			bFoundGeomObject = TRUE;
		break;
		}
	break;
	case GEOMOBJECT_CLASS_ID:
		bFoundGeomObject = TRUE;
	break;
	default:
		break;
	}

	return bFoundGeomObject;
}
//-----------------------------------------------------------------------------

CExporter::~CExporter()
{
	for (BoneDefIt b_it=m_Bones.begin(); b_it!=m_Bones.end(); b_it++){
		xr_delete(*b_it);
	}
	for (VertexDefIt v_it=m_Vertices.begin(); v_it!=m_Vertices.end(); v_it++){
		xr_delete(*v_it);
	}
	for (ExpVertIt ev_it=m_ExpVertices.begin(); ev_it!=m_ExpVertices.end(); ev_it++){
		xr_delete(*ev_it);
	}
	for (ExpFaceIt ef_it=m_ExpFaces.begin(); ef_it!=m_ExpFaces.end(); ef_it++){
		xr_delete(*ef_it);
	}
}
//-----------------------------------------------------------------------------
int	CExporter::AddBone(INode* pNode, Matrix3 &matMesh, IPhysiqueExport* pExport)
{
	if (!Helper::IsBone(pNode,U.m_SkinAllowDummy)) return BONE_NONE;

	for (BoneDefIt it=m_Bones.begin(); it!=m_Bones.end(); it++)
		if ((*it)->isEqual(pNode)) return it-m_Bones.begin();
	
	CBoneDef*	pBone= xr_new<CBoneDef>(pNode);
	if (pBone->SetInitTM(pExport,matMesh)){
		AddBone(pBone->pBone->GetParentNode(),matMesh,pExport);
		m_Bones.push_back	(pBone);
		return m_Bones.size()-1;
	}
	return BONE_NONE;
}

void CExporter::ScanBones(INode *pNode)
{
	VERIFY				(m_Style!=eExportUndef);
	R_ASSERT			(pNode);
	if (Helper::IsBone(pNode,U.m_SkinAllowDummy))	m_AllBones.push_back(pNode);
	// process sub-nodes
	int	cChildren		= pNode->NumberOfChildren();
	for (int iChild = 0; iChild < cChildren; iChild++)
		ScanBones(pNode->GetChildNode(iChild));
}

//-----------------------------------------------------------------------------
void CExporter::ScanMesh(INode *pNode)
{
	if (m_bHasError)	return;

	if (!Helper::IsMesh(pNode)){
//		LPCSTR nm = pNode->GetName();
		int	cChildren	= pNode->NumberOfChildren();
		for (int iChild = 0; iChild < cChildren; iChild++){
//			LPCSTR nm1	= pNode->GetChildNode(iChild)->GetName();
			ScanMesh	(pNode->GetChildNode(iChild));
		}
	}else{
//		LPCSTR nm = pNode->GetName();
		if (pNode->Selected()){
			if (m_bFindMesh){
				ERR("Single mesh support only.");
				m_bHasError = TRUE; 
				return;
			}
			m_MeshNode		= pNode;
			m_bFindMesh		= TRUE;
		}
	}
}
//-----------------------------------------------------------------------------
BOOL CExporter::Capture()
{
	VERIFY				(m_Style!=eExportUndef);
	Modifier*			pPhysique;
	IPhysiqueExport*	pExport;
	IPhyContextExport*	pContext;
	Object*				pObject;

	Matrix3				matMesh;
	Matrix3				matZero;

	if (!m_MeshNode){
		ERR("Select mesh and try again.");
		m_bHasError=TRUE; 
		return FALSE;
	}

	
	pObject				= m_MeshNode->GetObjectRef();
	if (!IsExportableMesh(m_MeshNode,pObject)){
		ERR("Can't receive node references.");
		m_bHasError=TRUE; 
		return FALSE;
	}

	// Get export interface
	pPhysique			= FindPhysiqueModifier(m_MeshNode);
	if (!pPhysique){
		ERR("Can't find Physique modifier.");
		m_bHasError=TRUE; 
		return FALSE;
	}
	pExport		= (IPhysiqueExport *)pPhysique->GetInterface(I_PHYINTERFACE);
	if (!pExport){
		ERR("Can't find Physique interface.");
		m_bHasError=TRUE; 
		return FALSE;
	}

	// Get mesh initial transform (used to mult by the bone matrices)
	int rval = CGINTM(m_MeshNode,pExport->GetInitNodeTM(m_MeshNode, matMesh));
	matZero.Zero();
	if (rval || matMesh.Equals(matZero, 0.0)){
		ERR("Old CS version. Can't export mesh");
		matMesh.IdentityMatrix();
	}

	// Add hierrarhy parts that has no effect on vertices, 
	// but required for hierrarhy stability

	if (eExportMotion==m_Style){
		if (m_AllBones.empty()){
			ERR("Invalid skin object. Bone not found.");
			return FALSE;
		}

		EConsole.ProgressStart((float)m_AllBones.size(),"..Capturing bones");
		for (DWORD i=0; i<m_AllBones.size(); i++){
			AddBone(m_AllBones[i], matMesh, pExport);
			EConsole.ProgressInc();
		}
		EConsole.ProgressEnd();
	}

	bool bRes = TRUE;
	if (eExportSkin==m_Style){
		// For a given Object's INode get a
		// ModContext Interface from the Physique Export Interface:
		pContext = (IPhyContextExport *)pExport->GetContextInterface(m_MeshNode);
		if (!pContext){
			ERR("Can't find Physique context interface.");
			return FALSE;
		}

		// convert to rigid with blending
		pContext->ConvertToRigid(TRUE);
		pContext->AllowBlending	(TRUE);

		// process vertices
		int	numVertices = pContext->GetNumberVertices();
		EConsole.ProgressStart(float(numVertices),"..Capturing vertices");
		for (int iVertex = 0; iVertex < numVertices; iVertex++ ){
			IPhyVertexExport *pVertexExport = (IPhyVertexExport *)pContext->GetVertexInterface(iVertex);
			R_ASSERT(pVertexExport);

			// What kind of vertices are these?
			int iVertexType = pVertexExport->GetVertexType();

			IPhyRigidVertex* pRigidVertex=(IPhyRigidVertex*)pContext->GetVertexInterface(iVertex);
			R_ASSERT					(pRigidVertex);
			switch (iVertexType){
			case RIGID_TYPE:{			
				INode* node				= pRigidVertex->GetNode(); 
				R_ASSERT				(node);
				LPCSTR nm				= node->GetName();
				// get bone and create vertex
				CVertexDef* pVertex		= AddVertex();
				int boneId				= AddBone(node,matMesh,pExport);
				if(BONE_NONE==boneId){
					ERR					("Invalid bone: ",node->GetName());
					bRes				= FALSE;
				}else pVertex->Append	(boneId,1.f);
							}break;
			case RIGID_BLENDED_TYPE:{
				IPhyBlendedRigidVertex*	pBlendedRigidVertex=(IPhyBlendedRigidVertex*)pRigidVertex;
				int cnt					= pBlendedRigidVertex->GetNumberNodes();
				CVertexDef* pVertex		= AddVertex();
				for (int i=0; i<cnt; i++){
					INode* node			= pBlendedRigidVertex->GetNode(i); 
					R_ASSERT			(node);
					LPCSTR nm			= node->GetName();
					// get bone and create vertex
					int boneId			= AddBone(node,matMesh,pExport);
					if(BONE_NONE==boneId){
						ERR				("Invalid bone: ",node->GetName());
						bRes			= FALSE;
					}else pVertex->Append(boneId,pBlendedRigidVertex->GetWeight(i));
				}
									}break;
			}

			// release vertex
			pContext->ReleaseVertexInterface( pRigidVertex );

			EConsole.ProgressInc();

			if (!bRes) break;
		}
		EConsole.ProgressEnd();

		if (!bRes) return FALSE;

		static int remap[3];
		if (U.m_SkinFlipFaces){
			remap[0] = 0;
			remap[1] = 1;
			remap[2] = 2;
		}else{
			remap[0] = 0;
			remap[1] = 2;
			remap[2] = 1;
		}

		// Process mesh
		// Get object from node. Abort if no object.
		Log("..Transforming mesh");
		BOOL		bDeleteTriObject;
		R_ASSERT	(pObject);
		TriObject *	pTriObject	= GetTriObjectFromObjRef(pObject, &bDeleteTriObject);
		if (!pTriObject){
			ERR("Can't create tri object.");
			return FALSE;
		}
		Mesh&		M = pTriObject->mesh;

		// Vertices
		{
			// check match with
			int iNumVert = M.getNumVerts();
			if (!(iNumVert==numVertices && iNumVert==m_Vertices.size()))
			{
				ERR("Non attached vertices found.");
				if (bDeleteTriObject)	delete(pTriObject);
				return FALSE;
			}

			// transform vertices
			for (int i=0; i<iNumVert; i++){
				Point3 P = M.getVert(i);
				Point3 T = matMesh.PointTransform(P);
				T *= m_fGlobalScale;
				m_Vertices[i]->SetPosition(T);
			}
		}

		Log("..Parsing materials");
		// Parse Materials
		m_MtlMain = m_MeshNode->GetMtl();
		R_ASSERT(m_MtlMain);

		DWORD cSubMaterials=m_MtlMain->NumSubMtls();
		if (cSubMaterials < 1) {
			// Count the material itself as a submaterial.
			cSubMaterials = 1;
		}

		// build normals
		M.buildRenderNormals();

		Log("..Converting vertices");
		// our Faces and Vertices
		{
			for (int i=0; i<M.getNumFaces(); i++){
				Face*	gF	= M.faces  + i;
				TVFace*	tF	= M.tvFace + i;

				int m_id = gF->getMatID();
				if (cSubMaterials == 1){
					m_id = 0;
				}else{
					// SDK recommends mod'ing the material ID by the valid # of materials, 
					// as sometimes a material number that's too high is returned.
					m_id %= cSubMaterials;
				}

				st_FACE* nF		= xr_new<st_FACE>();
				nF->m_id		= m_id;
				nF->sm_group	= gF->getSmGroup();
				for (int VVV=0; VVV<3; VVV++){
					int vert_id = gF->v[remap[VVV]];

					CVertexDef	&D	= *(m_Vertices[vert_id]);
					Point3		&UV	= M.tVerts[tF->t[remap[VVV]]];

					st_VERT		v;
					v.Set		(D);
					v.P.set		(D.P); 
					v.SetUV		(UV.x,1-UV.y);
//					v.sm_group	= U.m_SkinSuppressSmoothGroup?0:gF->getSmGroup(); // smooth group
					nF->v[VVV]	= AddVert(v);
				}
				m_ExpFaces.push_back(nF);
			}
		}
		if (bDeleteTriObject)	delete(pTriObject);
	}
	UpdateParenting();

	return bRes;
};

BOOL MeshExpUtility::SaveAsSkin(const char* fname)
{
	BOOL bRes=true;
	if (!(fname&&fname[0])) return false;

	Log				("Exporting..." );
	Log				("-------------------------------------------------------" );

	INode			*pRootNode;
	pRootNode		= ip->GetRootNode();
	R_ASSERT		(pRootNode);

	// export
	CExporter *E	= xr_new<CExporter>();
	bRes			= E->ExportSkin(pRootNode,fname);
	xr_delete		(E);

	return			bRes;
}
//-------------------------------------------------------------------

BOOL MeshExpUtility::SaveSkinKeys(LPCSTR fname){
	BOOL bRes=true;
	if (!(fname&&fname[0])) return false;

	Log				("Exporting Skin Keys..." );
	Log				("-------------------------------------------------------" );

	INode			*pRootNode;
	pRootNode		= ip->GetRootNode();
	R_ASSERT		(pRootNode);

	// export
	CExporter *E	= xr_new<CExporter>();
	bRes			= E->ExportMotion(pRootNode,fname);
	xr_delete(E);

	return			bRes;
}
//-------------------------------------------------------------------
