#include "stdafx.h"
#pragma hdrstop

#include "maTranslator.h"
#include "..\..\..\editors\Ecore\editor\EditObject.h"
#include "..\..\..\editors\Ecore\editor\EditMesh.h"
#include "smoth_flags.h"
//-----------------------------------------------------------------------------------------
BOOL CEditableObject::ParseMAMaterial(CSurface* dest, SXRShaderData& d)
{
	string1024 tmp;
	strcpy_s				(tmp, d.tex_name.asChar());	
	if (strext(tmp)) *strext(tmp)=0;

	dest->SetTexture	(EFS.AppendFolderToName(tmp, sizeof(tmp), 1, TRUE));
	dest->SetFVF		(D3DFVF_XYZ|D3DFVF_NORMAL|(1<<D3DFVF_TEXCOUNT_SHIFT));
	dest->SetVMap		("Texture");
	dest->m_Flags.set	(CSurface::sf2Sided,d.double_side);
	LPCSTR sh_name		= _ChangeSymbol(strcpy(tmp,d.eng_name.asChar()),'/','\\');
	if (!sh_name||!sh_name[0]){
		Log("!Empty shader name, material: ",d.name.asChar());
		return FALSE;
	}
	dest->SetShader		(sh_name);
	dest->SetShaderXRLC	(_ChangeSymbol(strcpy(tmp,d.comp_name.asChar()),'/','\\'));
	dest->SetGameMtl	(_ChangeSymbol(strcpy(tmp,d.gmat_name.asChar()),'/','\\'));
	return TRUE;
}

CSurface* CEditableObject::CreateSurface(LPCSTR m_name, SXRShaderData& d)
{
	CSurface* S			= FindSurfaceByName(m_name);
	if (!S){
		S				= xr_new<CSurface>();
		S->SetName		(m_name);
		if (!ParseMAMaterial(S,d)){ xr_delete(S); return 0; }
		m_Surfaces.push_back(S);
	}
	return S;
}

MStatus CXRayObjectExport::ExportAll(CEditableObject* O)
{
	MStatus status		= MS::kSuccess;

	if (initializeSetsAndLookupTables( true )){
		MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &status);

		if ( MS::kSuccess != status) {
			fprintf(stderr,"Failure in DAG iterator setup.\n");
			return MS::kFailure;
		}

		for ( ; !dagIterator.isDone(); dagIterator.next() ){
			MDagPath dagPath;
			MObject  component = MObject::kNullObj;
			status = dagIterator.getPath(dagPath);

			if (!status) {
				fprintf(stderr,"Failure getting DAG path.\n");
				freeLookupTables();
				return MS::kFailure;
			}

			// skip over intermediate objects
			//
			MFnDagNode dagNode( dagPath, &status );
			if (dagNode.isIntermediateObject()) continue;

			if ((dagPath.hasFn(MFn::kNurbsSurface)) && (dagPath.hasFn(MFn::kTransform))){
				status = MS::kSuccess;
				fprintf(stderr,"Warning: skipping Nurbs Surface.\n");
			}else if ((dagPath.hasFn(MFn::kMesh)) && (dagPath.hasFn(MFn::kTransform))){
				// We want only the shape, 
				// not the transform-extended-to-shape.
				continue;
			}else if (dagPath.hasFn(MFn::kMesh)){
				// Build a lookup table so we can determine which 
				// polygons belong to a particular edge as well as
				// smoothing information
				//
				buildEdgeTable( dagPath );

				// Now output the polygon information
				//
				status = ExportPart(O, dagPath, component);
				objectId++;
				if (status != MS::kSuccess) {
					fprintf(stderr,"Error: exporting geom failed.\n");
					freeLookupTables();                
					destroyEdgeTable(); // Free up the edge table				
					return MS::kFailure;
				}
				destroyEdgeTable(); // Free up the edge table
			}
		}
	}else{
		status = MS::kFailure;
	}

	freeLookupTables();

	return status;
}
//-----------------------------------------------------------------------------------------

MStatus CXRayObjectExport::ExportSelected(CEditableObject* O)
{
	MStatus status;
	MString filename;

	if (initializeSetsAndLookupTables( false )){
		// Create an iterator for the active selection list
		//
		MSelectionList slist;
		MGlobal::getActiveSelectionList( slist );
		MItSelectionList iter( slist );

		if (iter.isDone()){
			fprintf(stderr,"Error: Nothing is selected.\n");
			return MS::kFailure;
		}

		// We will need to interate over a selected node's heirarchy 
		// in the case where shapes are grouped, and the group is selected.
		MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &status);

		// Selection list loop
		for ( ; !iter.isDone(); iter.next())
		{
			MDagPath objectPath;
			// get the selected node
			status = iter.getDagPath( objectPath);

			// reset iterator's root node to be the selected node.
			status = dagIterator.reset (objectPath.node(), 
				MItDag::kDepthFirst, MFn::kInvalid );	

			// DAG iteration beginning at at selected node
			for ( ; !dagIterator.isDone(); dagIterator.next()){
				MDagPath dagPath;
				MObject  component = MObject::kNullObj;
				status = dagIterator.getPath(dagPath);

				if (!status) {
					fprintf(stderr,"Failure getting DAG path.\n");
					freeLookupTables();
					return MS::kFailure;
				}

				if (status ){
					// skip over intermediate objects
					//
					MFnDagNode dagNode( dagPath, &status );
					if (dagNode.isIntermediateObject()) 
						continue;

					if (dagPath.hasFn(MFn::kNurbsSurface)){
						status = MS::kSuccess;
						fprintf(stderr,"Warning: skipping Nurbs Surface.\n");
					}else if ((  dagPath.hasFn(MFn::kMesh)) && ( dagPath.hasFn(MFn::kTransform))){
						// We want only the shape, 
						// not the transform-extended-to-shape.
						continue;
					}else if (  dagPath.hasFn(MFn::kMesh)){
						// Build a lookup table so we can determine which 
						// polygons belong to a particular edge as well as
						// smoothing information
						//
						buildEdgeTable( dagPath );

						status = ExportPart(O, dagPath, component);
						objectId++;
						if (status != MS::kSuccess) {
							fprintf(stderr, "Error: exporting geom failed, check your selection.\n");
							freeLookupTables();
							destroyEdgeTable(); // Free up the edge table				
							return MS::kFailure;
						}
						destroyEdgeTable(); // Free up the edge table				
					}
				}
			}
		}
	}else{
		status = MS::kFailure;
	}
	
	freeLookupTables();
	
	return status;
}
//-----------------------------------------------------------------------------------------
typedef xr_map<int, int> PtLookupMap;
int AppendVertex(FvectorVec& _points, MPoint& _pt)
{
	Fvector pt; 
	// convert from internal units to the current ui units
	MDistance dst_x	(_pt.x);
	MDistance dst_y	(_pt.y);
	MDistance dst_z	(_pt.z);
	pt.set		((float)dst_x.asMeters(),(float)dst_y.asMeters(),-(float)dst_z.asMeters());

	for (FvectorIt it=_points.begin(); it!=_points.end(); it++)
		if (it->similar(pt)) return it-_points.begin();
	_points.push_back(pt);
	return _points.size()-1;
}
int AppendUV(st_VMap*& VM, Fvector2& _uv)
{
	int sz = VM->size();
	VM->appendUV(_uv);
	return sz;
}


MStatus CXRayObjectExport::set_smoth_flags( u32 &flags, const MIntArray& tri_vert_indeces )//  const MFnMesh &fnMesh, const MItMeshPolygon &meshPoly,
{
	return t_set_smoth_flags( *this, flags, tri_vert_indeces );
}

MStatus CXRayObjectExport::ExportPart(CEditableObject* O, MDagPath& mdagPath, MObject&  mComponent)
{
	MStatus stat = MS::kSuccess;
	MSpace::Space space = MSpace::kWorld;

	MFnMesh fnMesh( mdagPath, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MFnMesh initialization.\n");
		return MS::kFailure;
	}

	MString mdagPathNodeName = fnMesh.name();

	MFnDagNode dagNode1(mdagPath);
	u32 pc = dagNode1.parentCount();
	for(u32 ip=0;ip<pc;++ip)
	{
		MObject object_parent = dagNode1.parent(ip, &stat);
		if(object_parent.hasFn(MFn::kTransform))
		{
			MFnTransform parent_transform(object_parent,&stat);
	
			if ( MS::kSuccess == stat) 
			{
				mdagPathNodeName = parent_transform.name();
				break;
			}
		}
	}

	MItMeshPolygon meshPoly( mdagPath, mComponent, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MItMeshPolygon initialization.\n");
		return MS::kFailure;
	}

	MItMeshVertex vtxIter( mdagPath, mComponent, &stat );
	if ( MS::kSuccess != stat) {
		fprintf(stderr,"Failure in MItMeshVertex initialization.\n");
		return MS::kFailure;
	}

	// If the shape is instanced then we need to determine which
	// instance this path refers to.
	//
	int instanceNum = 0;
	if (mdagPath.isInstanced())
		instanceNum = mdagPath.instanceNumber();

	// Get a list of all shaders attached to this mesh
	MObjectArray rgShaders;
	MIntArray texMap;
	MStatus status;
	status = fnMesh.getConnectedShaders (instanceNum, rgShaders, texMap);
	if (status == MStatus::kFailure)
	{
		Log("!Unable to load shaders for mesh");
		return (MStatus::kFailure);
	}

	XRShaderDataVec xr_data;
	{
		for ( int i=0; i<(int)rgShaders.length(); i++ ) {
			MObject shader = rgShaders[i];

			xr_data.push_back(SXRShaderData());
			SXRShaderData& D = xr_data.back();

			status = parseShader(shader, D);
			if (status == MStatus::kFailure) {
				status.perror ("Unable to retrieve filename of texture");
				continue;
			}
		}
	}

	CEditableMesh* MESH = xr_new<CEditableMesh>(O);
	MESH->SetName(mdagPathNodeName.asChar());
	O->AppendMesh(MESH);

	int objectIdx, length;

	// Find i such that objectGroupsTablePtr[i] corresponds to the
	// object node pointed to by mdagPath
	length = objectNodeNamesArray.length();
	{
		for( int i=0; i<length; i++ ) {
			if( objectNodeNamesArray[i] == mdagPathNodeName ) {
				objectIdx = i;
				break;
			}
		}
	}
	// Reserve uv table
	{
		VMapVec& _vmaps	= MESH->m_VMaps;
		_vmaps.resize	(1);
		st_VMap*& VM	= _vmaps.back();
		VM				= xr_new<st_VMap>("Texture",vmtUV,false);
	}

	// write faces
	{
		DEFINE_VECTOR(st_Face,FaceVec,FaceIt);

		VMapVec& _vmaps			= MESH->m_VMaps;
		SurfFaces& _surf_faces	= MESH->m_SurfFaces;
		VMRefsVec& _vmrefs		= MESH->m_VMRefs;
		
		// temp variables
		FvectorVec	_points;
		FaceVec _faces;
		U32Vec _sgs;

		int f_cnt				= fnMesh.numPolygons();

		_sgs.reserve	(f_cnt);
		_faces.reserve	(f_cnt);
		_vmrefs.reserve	(f_cnt*3);

//		int lastSmoothingGroup = INITIALIZE_SMOOTHING;
		MPointArray rgpt;
		MIntArray rgint;

		PtLookupMap ptMap;
		CSurface* surf	= 0;
		for ( ; !meshPoly.isDone(); meshPoly.next()){
			// Write out the smoothing group that this polygon belongs to
			// We only write out the smoothing group if it is different
			// from the last polygon.
			//
			int compIdx	= meshPoly.index();
			int smoothingGroup = polySmoothingGroups[ compIdx ];
			// for each polygon, first setup the reverse mapping
			// between object-relative vertex indices and face-relative
			// vertex indices
			ptMap.clear();
			for (int i=0; i<(int)meshPoly.polygonVertexCount(); i++)
				ptMap.insert (PtLookupMap::value_type(meshPoly.vertexIndex(i), i) );

			// verify polygon zero area
			if (meshPoly.zeroArea()){
				status = MS::kFailure;
				Log("!polygon have zero area:",meshPoly.index());
				return status;
			}

			// verify polygon zero UV area
/*			if (meshPoly.zeroUVArea()){
				status = MS::kFailure;
				Log("!polygon have zero UV area:",meshPoly.index());
				return status;
			}
*/
			// verify polygon has UV information
			if (!meshPoly.hasUVs (&status)) {
				status = MS::kFailure;
				Log("!polygon is missing UV information:",meshPoly.index());
				return status;
			}

			int cTri;
			// now iterate through each triangle on this polygon and create a triangle object in our list
			status = meshPoly.numTriangles (cTri);	
			if (!status) {
				Log("!can't getting triangle count");
				return status;
			}

			for (int i=0; i < cTri; i++) {

				// for each triangle, first get the triangle data
				rgpt.clear();//triangle vertices
				rgint.clear();//triangle vertex indices 

				// triangles that come from object are retrieved in world space
				status = meshPoly.getTriangle (i, rgpt, rgint, MSpace::kWorld);

				if (!status) {
					Log("can't getting triangle for mesh poly");
					return status;
				}

				if ((rgpt.length() != 3) || (rgint.length() != 3)) {
					Msg("!3 points not returned for triangle");
					return MS::kFailure;
				}

				// Write out vertex/uv index information
				//
				R_ASSERT2(fnMesh.numUVs()>0,"Can't find uvmaps.");
				_faces.push_back(st_Face());
				_sgs.push_back(smoothingGroup);
				//set_smooth
				set_smoth_flags( _sgs.back(), rgint );

				st_Face& f_it		= _faces.back();
				for ( int vtx=0; vtx<3; vtx++ ) {
					// get face-relative vertex
					PtLookupMap::iterator mapIt;

					int vtLocal, vtUV;
					int vt = rgint[vtx];
					mapIt = ptMap.find(vt);
					Fvector2 uv;
					if (mapIt == ptMap.end()){
						Msg("!Can't find local index.");
						return MS::kFailure;
					}
					vtLocal = (*mapIt).second;

					status = meshPoly.getUVIndex (vtLocal, vtUV, uv.x, uv.y); 
					if (!status) {
						Msg("!error getting UV Index for local vertex '%d' and object vertex '%d'",vtLocal,vt);
						return status;
					}

					// flip v-part 
					uv.y=1.f-uv.y;

					f_it.pv[2-vtx].pindex	= AppendVertex(_points,rgpt[vtx]);
					f_it.pv[2-vtx].vmref	= _vmrefs.size();
					_vmrefs.push_back		(st_VMapPtLst());
					st_VMapPtLst& vm_lst	= _vmrefs.back();
					vm_lst.count			= 1;
					vm_lst.pts				= xr_alloc<st_VMapPt>(vm_lst.count);
					vm_lst.pts[0].vmap_index= 0;
					vm_lst.pts[0].index 	= AppendUV(_vmaps.back(),uv);
				}
				// out face material
				int iTexture	= texMap[meshPoly.index()];
				if (iTexture<0)
					Debug.fatal(DEBUG_INFO,"Can't find material for polygon: %d",meshPoly.index());
				SXRShaderData& D= xr_data[iTexture];

				int compIdx = meshPoly.index();
				surf		= MESH->Parent()->CreateSurface(getMaterialName(mdagPath, compIdx, objectIdx),D);
				if (!surf)	return MStatus::kFailure;	
				_surf_faces[surf].push_back(_faces.size()-1);
			}
		}
		{
			// copy from temp
			MESH->m_VertCount	= _points.size();
			MESH->m_FaceCount	= _faces.size();
			MESH->m_Vertices	= xr_alloc<Fvector>(MESH->m_VertCount);
			Memory.mem_copy		(MESH->m_Vertices,&*_points.begin(),MESH->m_VertCount*sizeof(Fvector));
			MESH->m_Faces		= xr_alloc<st_Face>(MESH->m_FaceCount);
			Memory.mem_copy		(MESH->m_Faces,&*_faces.begin(),MESH->m_FaceCount*sizeof(st_Face));
			MESH->m_SmoothGroups = xr_alloc<u32>(MESH->m_FaceCount);
			Memory.mem_copy		(MESH->m_SmoothGroups,&*_sgs.begin(),MESH->m_FaceCount*sizeof(u32));

			MESH->RecomputeBBox	();
		}
		if ((MESH->GetVertexCount()<4)||(MESH->GetFaceCount(true, false)<2))
		{
			Log		("!Invalid mesh: '%s'. Faces<2 or Verts<4",*MESH->Name());
			return MS::kFailure;
		}
	}
	return stat;
}
//-----------------------------------------------------------------------------------------
LPCSTR CXRayObjectExport::getMaterialName(MDagPath & mdagPath, int cid, int objectIdx)
{
    MStatus stat;
	
    int i, length;
	MIntArray * currentMaterials = xr_new<MIntArray>();
	MStringArray mArray;


	for ( i=0; i<numSets; i++ )	{
		if ( lookup(mdagPath,i,cid) ) {
			MFnSet fnSet( (*sets)[i] );
			if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) ) {
				currentMaterials->append( i );
				mArray.append( fnSet.name() );
			}
		}
	}

	// Test for equivalent materials
	//
	bool materialsEqual = false;
	if ((lastMaterials != NULL) && (lastMaterials->length() == currentMaterials->length())){
		materialsEqual = true;
		length = lastMaterials->length();
		for (i=0; i<length; i++){
			if ((*lastMaterials)[i]!=(*currentMaterials)[i]){
				materialsEqual = false;
				break;
			}
		}			
	}

	if (!materialsEqual){
		if (lastMaterials!=NULL) xr_delete(lastMaterials);

		lastMaterials = currentMaterials;

		int mLength = mArray.length(); 
		if (mLength==0) Debug.fatal(DEBUG_INFO,"Object '%s' has polygon '%d' without material.",0,cid);
		if (mLength>1){
			Debug.fatal(DEBUG_INFO,"Object '%s' has polygon '%d' with more than one material.",0,cid);
		}
	}else{
		xr_delete(currentMaterials);
	}
	return mArray[0].asChar();
}