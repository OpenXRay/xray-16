#include "stdafx.h" 

#include "SmdMaya.h"
#include "..\..\..\editors\Ecore\editor\EditObject.h"
#include "..\..\..\editors\Ecore\editor\EditMesh.h"
#include "Motion.h"
#include "Envelope.h"
#include "smoth_flags.h"

#pragma warning ( disable : 4786 )

//using namespace std;
#define NO_SMOOTHING_GROUP      -1
#define INITIALIZE_SMOOTHING    -2
#define INVALID_ID              -1

CXRaySkinExport::CXRaySkinExport(bool bReference)
{
	m_bReferenceFile		= bReference;
	polySmoothingGroups		= 0;
	edgeTable				= 0;
	m_rgWeights				= 0;
};

CXRaySkinExport::~CXRaySkinExport()
{
}

////////////////////////////////////////////////////
// set up the plug-in

//////////////////////////////////////////////////////////////

MString CXRaySkinExport::filter()const
{
	std::cerr <<"Dbg : MString CXRaySkinExport::filter()const\n";
	return m_bReferenceFile?"*.ob*":"*.skl";	
}

bool CXRaySkinExport::haveWriteMethod () const
{
	std::cerr <<"Dbg : bool CXRaySkinExport::haveWriteMethod () const\n";
    return true;
}

/*
MString CXRaySkinExport::defaultExtension () const
{
	std::cerr <<"Dbg : MString CXRaySkinExport::defaultExtension () const\n";
	return m_bReferenceFile?"object":"skl";	
}
*/
MPxFileTranslator::MFileKind CXRaySkinExport::identifyFile (const MFileObject& fileName, const char* buffer, short size) const
{
    const char * name = fileName.name().asChar();
    int   nameLength = xr_strlen(name);
    
	if (m_bReferenceFile){
		if ((nameLength > 7) && !strcasecmp(name+nameLength-7, ".object"))		
			return kCouldBeMyFileType;
		else
			return kNotMyFileType;
	}else{
		if ((nameLength > 4) && !strcasecmp(name+nameLength-7, ".skl"))
			return kCouldBeMyFileType;
		else
			return kNotMyFileType;
	}
}

void* CXRaySkinExport::creator_skin()
{
	return new CXRaySkinExport(true);
}

void* CXRaySkinExport::creator_skin_motion()
{
	return new CXRaySkinExport(false);
}
//////////////////////////////////////////////////////////////
// the key routine which does all the work
MStatus CXRaySkinExport::writer (const MFileObject& file, const MString& options, FileAccessMode mode)
{
    MStatus status;
	clearData();				// initialize all data structures

	bool b_ogf = false;
	MString locator_name;
	if (options.length() > 0) 
	{
        int i, length;
        // Start parsing.
        MStringArray optionList;
        MStringArray theOption;
        options.split(';', optionList); // break out all the options.

		length = optionList.length();
		for( i = 0; i < length; ++i )
		{
            theOption.clear();
            optionList[i].split( '=', theOption );
            if( theOption[0] == MString("SkinCluster") ) 
			{
				if (theOption.length() > 1 ) 
				{
					m_strSkinCluster = theOption[1];
				} else {
					m_strSkinCluster.clear();
				}
            }else
            if( theOption[0] == MString("locator") ) 
			{
				if(theOption.length() > 1)
					locator_name = theOption[1];
				else
					locator_name.clear();
			}else
            if( theOption[0] == MString("ogf") ) 
			{
				if(theOption.length() > 1)
					b_ogf = (theOption[1]==MString("on"));
			}

        }
	}

	// sanity check the options
	if (m_strSkinCluster.length() == 0) {
		m_fSkinCluster = false;
	} else {
		m_fSkinCluster = true;
	}
	MString mname;
	if (m_bReferenceFile){
		mname = file.fullName()+".object";
	}else{
		mname = file.fullName()+".skl";
	}
    LPCSTR fname = mname.asChar();    

	

	// now load the bones themselves and store the skeleton hierarchy in memory
	if (mode == kExportActiveAccessMode)
	{

		MSelectionList activeList;
		MGlobal::getActiveSelectionList(activeList);
		MItSelectionList iter( activeList);
		int count = 0;
		for ( ; !iter.isDone(); iter.next() ) {

			MObject	obj;
			MDagPath DagPath;
			iter.getDagPath(DagPath, obj);
			const char * aptTypePtr = obj.apiTypeStr();


			DagPath.extendToShape();
			const char* fullpath = DagPath.fullPathName().asChar();


			obj = DagPath.node();
			aptTypePtr = obj.apiTypeStr();
			//-------------------------------
			MStatus stat;
			MFnDagNode dagNode(DagPath);    // path to the visible mesh
			MFnMesh meshFn(DagPath, &stat);     // this is the visible mesh
			MObject inObj;
			MObject dataObj1;

			// the deformed mesh comes into the visible mesh
			// through its "inmesh" plug
			MPlug inMeshPlug = dagNode.findPlug("inMesh", &stat);

			if (stat == MS::kSuccess && inMeshPlug.isConnected())
			{
				// walk the tree of stuff upstream from this plug
				MItDependencyGraph dgIt(inMeshPlug,
					MFn::kInvalid,
					MItDependencyGraph::kUpstream,
					MItDependencyGraph::kDepthFirst,
					MItDependencyGraph::kPlugLevel,
					&stat);

				if (MS::kSuccess == stat)
				{
					dgIt.disablePruningOnFilter();
					int count = 0;

					for ( ; ! dgIt.isDone(); dgIt.next() )
					{
						MObject thisNode = dgIt.thisNode();
						aptTypePtr = thisNode.apiTypeStr();

						// go until we find a skinCluster

						if (thisNode.apiType() == MFn::kSkinClusterFilter)
						{
							MFnSkinCluster skinCluster(thisNode);
							const char* Name = skinCluster.name().asChar();
							Msg("Found Selected skin - %s", Name);
							m_fSkinCluster = true;
							m_strSkinCluster = skinCluster.name();

							status = setUpBoneMap(&(thisNode));
						}
					}
				}
			}
			//-------------------------------
		}
	}
	else
	{
		status = setUpBoneMap (NULL);
	}
	Msg("Starting skin export...");

	if (m_fSkinCluster)
		Msg("Trying to export mesh - %s",m_strSkinCluster.asChar());

	if (!status) {
		Msg("!problem setting up bone table");
		return status;
	}

	// first look through the scene for skin cluster objects. Find any bones used in these
	// clusters and load and store their weights in table by vertex.
	status = getBones();
	if (!status)
		Msg("!problem loading bone weights");

	// now look through the scene for any geometry directly attached to bones
	// this is most commonly found when creating mechanical models vs. organic models
	status = parseBoneGeometry ();
	if (!status)
		Msg("!problem loading skin information");

	// now load the geometry information for any skin cluster objects detected earlier
	if (m_skinPath.isValid()) {
		status = parseShape (m_skinPath);
		if (!status) {
			Msg("!problem loading skin information");
			return status;
		}
	}
	MMatrix Mlocator = MMatrix::identity;
	if(locator_name.length())
	{
		MSelectionList	L;
		L.add			(locator_name);

		MItSelectionList li (L);
		for ( ; !li.isDone(); li.next() ) 
		{
			
			MObject			obj_locator;
			MDagPath		DagPath;
			li.getDagPath	(DagPath, obj_locator);

			if( DagPath.hasFn(MFn::kLocator) )
			{
				MFnTransform	fn(DagPath.transform());
				MTransformationMatrix M = fn.transformation();
				Mlocator = M.asMatrix();
				break;
			}
			//.
		}
	}
	// now create the actual SMD file
	if(	(mode==MPxFileTranslator::kExportAccessMode)||
		(mode==MPxFileTranslator::kSaveAccessMode)||
		(mode==MPxFileTranslator::kExportActiveAccessMode))
		status = (m_bReferenceFile) ? exportObject(fname, b_ogf) : exportMotion(fname, Mlocator);
	if (MS::kSuccess==status){ 
		Msg("Object succesfully exported.");
	}else{
		Msg("!Export failed.");
	}
	return status;
}

MStatus CXRaySkinExport::reader (const MFileObject& file, const MString& options, FileAccessMode mode)
{
    Msg("!reader called in error.");
    return MS::kFailure;
}

//////////////////////////////////////////////////////////////

CSurface* CEditableObject::CreateSurface(MObject shader)
{
	CSurface* S			= 0;
	for(SurfaceIt s_it=m_Surfaces.begin(); s_it!=m_Surfaces.end(); s_it++)
		if ((*s_it)->tag==*((int*)&shader)) return *s_it;
	if (!S){
		S				= xr_new<CSurface>();
		S->tag			= *((int*)&shader);
		SXRShaderData	d;
		MStatus status	= parseShader(shader, d);
		if (status == MStatus::kFailure) {
			Msg			("!Unable to retrieve filename of texture");
			xr_delete	(S);
			return		0;
		}
		S->SetName		(d.name.asChar());
		if (!ParseMAMaterial(S,d)){ xr_delete(S); return 0; }
		m_Surfaces.push_back(S);
	}
	return S;
}

////////////////////////////////////////////////////
// routines for printing out animation data to files
//
// createFile sets up the file on disk and calls the other routines
// dumpHeader prints the header
// dumpTriangles prints out the triangle info
// dumpNodes prints out the list of bones
// dumpAnimation prints out animation for all frames of data
// dumpPose prints out the position & rotation of each bone
////////////////////////////////////////////////////

MStatus	CXRaySkinExport::gotoBindPose(void)
{
	MStatus status;
	if (m_bReferenceFile){
		for (SmdBoneIt itBones = m_boneList.begin(); itBones != m_boneList.end(); itBones++){
			if ((*itBones)->parentId==-1){
				MFnTransform fnJoint = (*itBones)->path.node(&status);
				if (!status) {
					Msg("!Unable to cast as Transform!");
					break;
				}
				MString cmd = "dagPose -r -g -bp "+fnJoint.fullPathName();
				MGlobal::executeCommand(cmd, status );
				if (!status) {
					Msg("!Unable goto bind pose!");
					break;
				}
			}
		}
	}
	return status;
}

MStatus CXRaySkinExport::exportObject(LPCSTR fn, bool b_ogf)
{
	MStatus status;
	MGlobal::executeCommand("DisableAll");

	R_ASSERT(m_bReferenceFile);


	string_path			fname;
	strcpy				(fname,fn);
	if (strext(fname)) 
		*strext(fname)=0;

	CEditableObject* OBJECT = xr_new<CEditableObject>(fname);
	OBJECT->SetVersionToCurrent(TRUE,TRUE);
	CEditableMesh* MESH		= xr_new<CEditableMesh>(OBJECT);
	MESH->SetName			("skin");
	OBJECT->Meshes().push_back(MESH);
	// IMPORT MESH
	SurfFaces&	_surf_faces	= MESH->m_SurfFaces;
	VMRefsVec&	_vmrefs		= MESH->m_VMRefs;
	VMapVec&	_vmaps		= MESH->m_VMaps;
	Fvector*&	_points		= MESH->m_Vertices;
	// temp variables
	st_Face*&	_faces		= MESH->m_Faces;
	u32*&		_sgs		= MESH->m_SmoothGroups;
	// maps
	// Weight maps 
	_vmaps.resize			(m_boneList.size()+1);
	for (DWORD b_i=0; b_i<m_boneList.size(); b_i++)
		_vmaps[b_i]			= xr_new<st_VMap>(m_boneList[b_i]->name,vmtWeight,false);
	// UV map
	int VM_UV_idx			= _vmaps.size()-1;
	st_VMap*& VM_UV			= _vmaps[VM_UV_idx];
	VM_UV					= xr_new<st_VMap>("texture",vmtUV,false);
	// Write out the vertex table
	{
		MESH->m_VertCount	= m_vertList.size();
		MESH->m_Vertices	= xr_alloc<Fvector>(MESH->m_VertCount);
		Fvector* pt_it		= _points;

		for (SmdVertIt it=m_vertList.begin(); it!=m_vertList.end(); it++,pt_it++){
			// convert from internal units to the current ui units
			MDistance dst_x	((*it)->pos.x);
			MDistance dst_y	((*it)->pos.y);
			MDistance dst_z	((*it)->pos.z);
			pt_it->set		((float)dst_x.asMeters(),(float)dst_y.asMeters(),(float)dst_z.asMeters());
			VM_UV->appendUV	((*it)->uv.x,(*it)->uv.y);
		}
	}

	// faces
	{
		// reserve space for faces and references
		MESH->m_FaceCount	= m_triList.size();
		_sgs				= xr_alloc<u32>(MESH->m_FaceCount);
		_faces				= xr_alloc<st_Face>(MESH->m_FaceCount);
		_vmrefs.resize		(m_vertList.size());

		int f_id			= 0;
		CSurface* surf		= 0;
		for (SmdTriIt it=m_triList.begin(); it!=m_triList.end(); it++,f_id++){
			// FACES
			_sgs[f_id]				= ((*it)->sm_group);

			//set_smooth
			//set_smoth_flags( _sgs.back(), rgint );

			st_Face& F				= _faces[f_id];
			for (int k=0; k<3; k++){
				int v_idx			= (*it)->v[k];
				st_FaceVert& vt		= F.pv[k];
				SmdVertex* V		= m_vertList[v_idx];
				vt.pindex			= v_idx;
				st_VMapPtLst& vm_lst= _vmrefs[vt.pindex];
				vm_lst.count		= V->influence.size()+1;
				vm_lst.pts			= xr_alloc<st_VMapPt>(vm_lst.count);
				vm_lst.pts[0].vmap_index= VM_UV_idx;
				vm_lst.pts[0].index 	= vt.pindex;
				for (WBIt vd_it=V->influence.begin(); vd_it!=V->influence.end(); vd_it++){
					u32 idx			= vd_it-V->influence.begin()+1;
					st_VMap* vm		= _vmaps[vd_it->bone];
					vm->appendW		(vd_it->weight);
					vm_lst.pts[idx].vmap_index	= vd_it->bone;
					vm_lst.pts[idx].index 		= vm->size()-1;
				}
				vt.vmref			= vt.pindex;
			}
			SXRShaderData D;
			surf		= MESH->Parent()->CreateSurface((*it)->shader);
			if (!surf)	return MStatus::kFailure;	
			_surf_faces[surf].push_back(f_id);
		}
	}

	// BONES
	OBJECT->Bones().reserve(m_boneList.size());
	status = getBoneData(MMatrix::identity);
	if (!status) 
	{
		Msg("!error getting bind pose.");
		MGlobal::executeCommand("doEnableNodeItems true all");
		return status;
	}
	for (SmdBoneIt boneIt = m_boneList.begin(); boneIt != m_boneList.end(); ++boneIt)
	{
		float length		= 0.5f;
		Fvector offset,rotate;
		offset.set			((*boneIt)->trans);
		rotate.set			((*boneIt)->orient);
		OBJECT->Bones().push_back(xr_new<CBone>());
		CBone* BONE			= OBJECT->Bones().back(); 
		BONE->SetWMap		((*boneIt)->name);
		BONE->SetName		((*boneIt)->name);
		BONE->SetParentName	((*boneIt)->parentId>-1?m_boneList[(*boneIt)->parentId]->name:0); //. need convert space
		BONE->SetRestParams	(length,offset,rotate);
	}

	// default bone part
	OBJECT->BoneParts().push_back(SBonePart());
	SBonePart& BP = OBJECT->BoneParts().back();
	BP.alias = "default";
	
	for (int b_i=0; b_i<(int)OBJECT->Bones().size(); ++b_i)
		BP.bones.push_back(OBJECT->Bones()[b_i]->Name());
	
	OBJECT->m_objectFlags.set(CEditableObject::eoDynamic, TRUE);

	if ((MESH->GetVertexCount()<4)||(MESH->GetFaceCount()<2))
	{
		Log		("!Invalid mesh: '%s'. Faces<2 or Verts<4",*MESH->Name());
		MGlobal::executeCommand("doEnableNodeItems true all");
		return MStatus::kFailure;
	}

	MESH->RecomputeBBox		();

	string_path				object_name;
	string_path				ogf_name;
	strconcat				(sizeof(object_name), object_name, fname, ".object");
	strconcat				(sizeof(ogf_name), ogf_name, fname, ".ogf");
	OBJECT->Optimize		();
	Msg						("Exporting to Object [%s]",object_name);
	OBJECT->Save			(object_name);
	Msg						("success.");
	if(b_ogf)
	{
		Msg						("Exporting to OGF [%s]",ogf_name);
		OBJECT->ExportOGF		(ogf_name, 4);
		Msg						("success.");
	}
	xr_delete				(OBJECT);

	MGlobal::executeCommand("doEnableNodeItems true all");

	return MStatus::kSuccess;
}

MStatus CXRaySkinExport::exportMotion(LPCSTR fn, const MMatrix& locator)
{
	R_ASSERT(!m_bReferenceFile);
	string256 nm;
	strcpy(nm,fn);
	if (strext(nm)) *strext(nm)=0;

	int		frameFirst;
	int		frameLast;
	MTime	tmNew;
	MStatus	status;

	// Remember the frame the scene was at so we can restore it later.
	MTime currentFrame	= MAnimControl::currentTime();
	MTime startFrame	= MAnimControl::minTime();
	MTime endFrame		= MAnimControl::maxTime();

	frameFirst			= (int) startFrame.as( MTime::uiUnit() );
	frameLast			= (int) endFrame.as( MTime::uiUnit() );
	tmNew.setUnit		(MTime::uiUnit());

	if (currentFrame.unit()!=MTime::kNTSCFrame){
		Msg("!Can't export animation with FPS!=30.f");
		return MStatus::kFailure;
	}
	float iFPS			= 30.f;

	// build motion
	CSMotion* MOT		= xr_new<CSMotion>();
	MOT->SetParam		(frameFirst,frameLast,(float)iFPS);
	MOT->SetName		(nm);

	// cout << "Exporting animation frames from " << frameFirst << " to " << frameLast << "\n";
	{
		BoneMotionVec& BMVec= MOT->BoneMotions();
		BMVec.reserve		(m_boneList.size());

		for (SmdBoneIt boneIt = m_boneList.begin(); boneIt != m_boneList.end(); boneIt++) 
		{
			SmdBone* bone	= *boneIt;

			BMVec.push_back	(st_BoneMotion());
			st_BoneMotion& BM = BMVec.back();
			BM.SetName		(bone->name);

			BM.envs[ctPositionX] = xr_new<CEnvelope>();
			BM.envs[ctPositionY] = xr_new<CEnvelope>();
			BM.envs[ctPositionZ] = xr_new<CEnvelope>();
			BM.envs[ctRotationH] = xr_new<CEnvelope>();
			BM.envs[ctRotationP] = xr_new<CEnvelope>();
			BM.envs[ctRotationB] = xr_new<CEnvelope>();

			BM.envs[ctPositionX]->behavior[0]=1; BM.envs[ctPositionX]->behavior[1]=1;
			BM.envs[ctPositionY]->behavior[0]=1; BM.envs[ctPositionY]->behavior[1]=1;
			BM.envs[ctPositionZ]->behavior[0]=1; BM.envs[ctPositionZ]->behavior[1]=1;
			BM.envs[ctRotationH]->behavior[0]=1; BM.envs[ctRotationH]->behavior[1]=1;
			BM.envs[ctRotationP]->behavior[0]=1; BM.envs[ctRotationP]->behavior[1]=1;
			BM.envs[ctRotationB]->behavior[0]=1; BM.envs[ctRotationB]->behavior[1]=1;
		}

		for (int i=frameFirst; i<=frameLast; i++)
		{
			tmNew.setValue (i);
			MGlobal::viewFrame( tmNew );

			status			= getBoneData(locator);
			if (!status){
				Msg("!error getting bone data at frame: ",i);
				continue;
			}

			float displacedTime = (float)tmNew.as(MTime::kSeconds);

			st_Key *X,*Y,*Z,*H,*P,*B;
			for (SmdBoneIt boneIt = m_boneList.begin(); boneIt != m_boneList.end(); boneIt++) 
			{
				SmdBone* bone	= *boneIt;
				st_BoneMotion& BM = BMVec[boneIt-m_boneList.begin()];

				X = xr_new<st_Key>();	Y = xr_new<st_Key>();	Z = xr_new<st_Key>();
				H = xr_new<st_Key>();	P = xr_new<st_Key>();	B = xr_new<st_Key>();
				BM.envs[ctPositionX]->keys.push_back(X);	
				BM.envs[ctPositionY]->keys.push_back(Y);	
				BM.envs[ctPositionZ]->keys.push_back(Z);
				BM.envs[ctRotationH]->keys.push_back(H);	
				BM.envs[ctRotationP]->keys.push_back(P);	
				BM.envs[ctRotationB]->keys.push_back(B);

				float h,p,b, x,y,z;
				x=bone->trans.x; 
				y=bone->trans.y; 
				z=bone->trans.z;
				h=bone->orient.y;
				p=bone->orient.x;
				b=bone->orient.z;

				X->time = displacedTime;	Y->time = displacedTime;	Z->time = displacedTime;
				H->time = displacedTime;	P->time = displacedTime;	B->time = displacedTime;
				
				X->shape = 4;	Y->shape = 4;	Z->shape = 4; 
				H->shape = 4;	P->shape = 4;	B->shape = 4;
				X->value = x;	Y->value = y;	Z->value = z;
				H->value = h;	P->value = p;	B->value = b;
			}
		}
		strcat(nm,".skl");
		MOT->SaveMotion(nm);
		xr_delete(MOT);
	}

	// now restore the original frame
	MGlobal::viewFrame( currentFrame );

	return MStatus::kSuccess;
}

int CXRaySkinExport::AppendVertex(MPoint pt, float u, float v, const WBVec& wb)
{
	for (SmdVertIt it=m_vertList.begin(); it!=m_vertList.end(); it++)
	{
		if ( (*it)->similar( pt.x, pt.y, pt.z, u, v, wb ) ) 
							return it-m_vertList.begin();
	}
	{
		m_vertList.push_back( xr_new<SmdVertex>( pt, u, v, wb ) );
		return m_vertList.size()-1;
	}
}

////////////////////////////////////////////////////
// routines for loading data from the Maya scene file
//
// pass in the array of u, v, and normals for this mesh
// look up these values using the vertices that come from the "get triangles" function
//
// note: there is one major hack needed in here
// getTriangle returns a list of object-relative vertices...
// unfortunately, the "GetNormal" and "GetUV" calls both require face-relative
// vertices... and there is no way to go from an object-relative vertex to
// a face-relative vertex. so I need to write my own (so broken!!!)
//
// if presetBone is set to anything other than -1 then it will force vertices to be set to it
// otherwise vertex influences are pulled from the mesh directly
////////////////////////////////////////////////////

MStatus CXRaySkinExport::parsePolySet(MItMeshPolygon &meshPoly, MObjectArray& rgShaders, MIntArray texMap, int presetBone = -1)
{
	SmdTriangle *tri;
	MPointArray rgpt;
	MIntArray rgint;
	MVector pt;
	float u,v;
	int	weight;

	MStatus status;
	int cTri;
//	u32 i;

	PtLookupMap ptMap;

	// cout << "Mesh has " << meshPoly.count() << " polygons\n";

	for ( ; !meshPoly.isDone(); meshPoly.next() ) {
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
		for (int i=0; i<(int)meshPoly.polygonVertexCount(); i++) {
			ptMap.insert (PtLookupMap::value_type(meshPoly.vertexIndex(i), i) );
		}
  		
		// verify polygon zero area
		if (meshPoly.zeroArea()){
			status = MS::kFailure;
			Log("!polygon have zero area:",meshPoly.index());
			return status;
		}

		// verify polygon zero UV area
/*		if (meshPoly.zeroUVArea()){
			status = MS::kFailure;
			Log("!polygon have zero UV area:",meshPoly.index());
			return status;
		}
*/
		// verify polygon has UV information
		if (!meshPoly.hasUVs ()) {
			status = MS::kFailure;
			Log("!polygon is missing UV information:",meshPoly.index());
			return status;
		}

		// now iterate through each triangle on this polygon and create a triangle object in our list
		status = meshPoly.numTriangles (cTri);	
		if (!status) {
			status.perror ("error getting triangle count");
			return status;
		}
		
		for (u32 i=0; i < u32(cTri); ++i) 
		{
			// for each triangle, first get the triangle data
			rgpt.clear();
			rgint.clear();

			// triangles that come from a skin are retrieved in world space
			// trianges that are directly attached are retrieved in object space
			if (presetBone > -1)
				status = meshPoly.getTriangle (i, rgpt, rgint, MSpace::kObject);
			else
				status = meshPoly.getTriangle (i, rgpt, rgint, MSpace::kWorld);

			if (!status) {
				status.perror ("error getting triangle for mesh poly");
				return status;
			}

			if ((rgpt.length() != 3) || (rgint.length() != 3)) {
				Msg("!3 points not returned for triangle");
				status = MS::kFailure;
				return status;
			}

			tri = xr_new<SmdTriangle>();
			if (tri == NULL) {
				Msg("!error creating triangle");
				status = MS::kFailure;
				return status;
			}

			// set the bitmap filename for the triangle
			int iMat = texMap[meshPoly.index()];
			if (iMat >= 0)
				tri->shader = rgShaders[iMat];

			WBVec wb;
			// VERTEX #1
			wb.clear();
			pt = rgpt[2];	pt.z = -pt.z;
			CalculateTriangleVertex (rgint[2], pt, u, v, wb, meshPoly, ptMap);
			if (presetBone > -1)
				weight = presetBone;
			tri->v[0] = AppendVertex(pt, u, 1.f-v, wb);

			// VERTEX #2
			wb.clear();
			pt = rgpt[1];	pt.z = -pt.z;
			CalculateTriangleVertex (rgint[1], pt, u, v, wb, meshPoly, ptMap);
			if (presetBone > -1)
				weight = presetBone;
			tri->v[1] = AppendVertex(pt, u, 1.f-v, wb);

			// VERTEX #3
			wb.clear();
			pt = rgpt[0];	pt.z = -pt.z;
			CalculateTriangleVertex (rgint[0], pt, u, v, wb, meshPoly, ptMap);
			if (presetBone > -1)
				weight = presetBone;
			tri->v[2] = AppendVertex(pt, u, 1.f-v, wb);

			tri->sm_group = smoothingGroup;
//
			//set_smooth
			t_set_smoth_flags(*this, tri->sm_group, rgint );
//
			m_triList.push_back (tri);
		}
	}

	return MS::kSuccess;
}

// this function is at the heart of the polygon parsing routine
// given a mesh-relative vertex index it returns all the information
// needed to fill in a single vertex of the triangle
MStatus CXRaySkinExport::CalculateTriangleVertex (int vt, MVector &pt, float &u, float &v, WBVec& weights, MItMeshPolygon &meshPoly, PtLookupMap &ptMap)
{
	MStatus	status;
	int vtLocal;
	int vtUV;

	// get face-relative vertex
	PtLookupMap::iterator mapIt;

	mapIt = ptMap.find(vt);
	if (mapIt != ptMap.end()) {
		vtLocal = (*mapIt).second;
	}

/*
	status = meshPoly.getNormal (vtLocal, n, MSpace::kWorld);
	if (!status) {
		Msg("!error getting normal for local vertex '%d' and object vertex '%d'",vtLocal,vt);
		return status;
	}
*/

	status = meshPoly.getUVIndex (vtLocal, vtUV, u, v);
	if (!status) {
		Msg("!error getting UV Index for local vertex '%d' and object vertex '%d'",vtLocal,vt);
		return status;
	}

	weights = (*m_rgWeights)[vt];
/*
//.
	// determine which bone affects this vertex
	if (!m_rgWeights.empty()) {
		weight = m_rgWeights[vt];

		// now need to look up appropriate bone for this vertex
		MDagPath path = m_rgInfs[weight];
		MFnDependencyNode joint (path.node());
		char *name = (char *)joint.name().asChar();
		NameIntMap::iterator mapIt2;
		mapIt2 = m_jointMap.find(name);
		if (mapIt2 != m_jointMap.end()) {
			weight = (*mapIt2).second;
		} else {
			// error - bone not found!
			Msg("!can't find weight bone with index '%d' and name '%s' in joint map.",weight,name);
			return MS::kFailure;
		}
	} else {
		weight = 0;
	}
*/

	return MS::kSuccess;
}

// iterate through entire scene and find all interesting objects
// looks for mesh objects. for each mesh objects, locate skin clusters
// for each skin cluster, find out its bones and other data
MStatus CXRaySkinExport::getBones(void)
{	
	size_t count = 0;

	// Iterate through graph and search for skinCluster nodes
	MItDependencyNodes iter( MFn::kSkinClusterFilter );
	for ( ; !iter.isDone(); iter.next() ) {
		MObject object = iter.item();
		count++;

		MFnSkinCluster skinCluster(object);

		// did the user specify a skin cluster?
		if (m_fSkinCluster) {
			// If so, is this not it?
			if (xr_strcmp(skinCluster.name().asChar(), m_strSkinCluster.asChar())) {
				continue;
			}
		} else {
			m_strSkinCluster = skinCluster.name();
		}

		// otherwise proceed with analyzing this skinCluster
		// get the list of influence objects. store all influence objects in the array "m_rgInfs"
		// only used if the user decides not to export all bones

		MStatus stat;
		unsigned int nInfs = skinCluster.influenceObjects(m_rgInfs, &stat);
		if (!stat) {
			stat.perror("Error getting influence objects");
			continue;
		}

		if (0 == nInfs) {
			stat = MStatus::kFailure;
			stat.perror("Error: No influence objects found.");
			return stat;
		}
			
		// loop through the geometries affected by this cluster
		unsigned int nGeoms = skinCluster.numOutputConnections();
		for (size_t ii = 0; ii < nGeoms; ++ii) {
			unsigned int index = skinCluster.indexForOutputConnection(ii,&stat);
			if (!stat) {
				Msg("!Error getting geometry index.");
				return stat;
			}

			// get the dag path of the ii'th geometry
			stat = skinCluster.getPathAtIndex(index,m_skinPath);
			if (!stat) {
				Msg("!Error getting geometry path.");
				return stat;
			}

			// iterate through the components of this geometry
			MItGeometry gIter(m_skinPath);

			// print out the path name of the skin, vertexCount & influenceCount
			// cout << "found skin: '" << m_skinPath.partialPathName().asChar() << "' with " << gIter.count() << " vertices " << "and " << nInfs << " influences\n";
							
			// set up the array for all vertices
			xr_delete(m_rgWeights);
			m_rgWeights = xr_new<VWBVec>();
			m_rgWeights->resize(gIter.count(&stat));
			if (!stat) {
				Msg("!Error creating array of vertices");
				return stat;
			}

			for (; !gIter.isDone(); gIter.next()) {
				MObject comp = gIter.component(&stat);
				if (!stat) {
					Msg("!Error getting component.");
					return stat;
				}

				// Get the weights for this vertex (one per influence object)
				MFloatArray wts;
				unsigned int infCount;
				stat = skinCluster.getWeights(m_skinPath,comp,wts,infCount);
				if (!stat) {
					Msg("!Error getting weights.");
					return stat;
				}

				if (0==infCount)	{ Msg("!0 influence objects.");	return MStatus::kFailure; }

				// find the strongest influencer for this vertex
				st_VertexWB& wb			= (*m_rgWeights)[gIter.index()]; 
				for (u32 iInf = 0; iInf < infCount ; ++iInf ) {
					if (!fis_zero(wts[iInf])){
						int bone_idx=-1;
						{
							int cnt = m_rgInfs.length();
							// now need to look up appropriate bone for this vertex
							MDagPath path = m_rgInfs[iInf];
							MFnDependencyNode joint (path.node());
							char *name = (char *)joint.name().asChar();
							NameIntMap::iterator mapIt2;
							mapIt2 = m_jointMap.find(name);
							if (mapIt2 != m_jointMap.end()) {
								bone_idx = (*mapIt2).second;
							} else {
								// error - bone not found!
								Msg("!can't find weight bone with name '%s' in joint map.",name);
								return MS::kFailure;
							}
						}
						if (bone_idx!=iInf){
							Msg("!bone_idx!=iInf.[%d-%d]",bone_idx,iInf);
						}
						wb.push_back	(st_WB(bone_idx,wts[iInf]));
					}
				}
				wb.prepare_weights(4/*2*/);
			}
		}
	}


	if (0 == count) {
		Msg("!No skinned meshes found in this scene. Is your mesh bound to a skeleton?");
		return MStatus::kFailure;
	} else if (m_skinPath.partialPathName().length() == 0) {
		Msg("!Could not find desired skin cluster '%s'",m_strSkinCluster.asChar());
		return MStatus::kFailure;
	} else {
		Msg("Processing skin cluster '%s'",m_strSkinCluster.asChar());
	}
	return MStatus::kSuccess;
}

// create the bone mapping that we will use to store animation data for each
// frame of animation
MStatus CXRaySkinExport::setUpBoneMap(MObject* pSkinObject)
{
	MStatus	status;
	int lastJointID = -1;
	m_jointMap.clear();

	if (!pSkinObject) 
	{
		// get list of all bones that are in this file
		MItDependencyNodes iter( MFn::kJoint);
		for ( ; !iter.isDone(); iter.next() ) 
		{
			MObject object	= iter.item();
			MDagPath		jointPath;

			MFnDagNode joint(object);
			char *name		= (char *)joint.name().asChar();

			NameIntMap::iterator mapIt;
			mapIt			= m_jointMap.find(name);

			if (mapIt == m_jointMap.end()) 
			{
				// this is a new bone; need to add it to the list
	
				SmdBone *bone		= xr_new<SmdBone>();
				bone->id			= ++lastJointID;
				bone->name			= xr_strdup(joint.name().asChar());
				joint.getPath		(jointPath);
				bone->path			= jointPath;

				// Add this joint to our joint map so we can look it up by name
				m_jointMap.insert	( NameIntMap::value_type(bone->name, bone->id) );

				// Pop one up the path stack to get our parent
				jointPath.pop		(1);

				MFnDagNode parentNode(jointPath, &status);
				if ( !status ) 
				{
					status.perror("MFnDagNode constructor");
					return status;
				}
				else 
				{
					char *parentName	= (char *)parentNode.name().asChar();
					mapIt				= m_jointMap.find(parentName);

					if (mapIt != m_jointMap.end()) 
						bone->parentId = (*mapIt).second;
				}	

				m_boneList.push_back(bone);
			}
		}
	} else 
	{
		MFnSkinCluster	skinCluster(*pSkinObject);

		MDagPathArray	tmp_rgInfs;
		MStatus stat;
		unsigned int nInfs = skinCluster.influenceObjects(tmp_rgInfs, &stat);
		if (!stat) 
		{
			stat.perror("Error getting influence objects");
			return MStatus::kFailure;
		}
	// now go through only the skinned bones...
		for (u32 i=0; i<tmp_rgInfs.length(); i++)
		{
			MDagPath jointPath			= tmp_rgInfs[i];

			SmdBone *bone				= new SmdBone;
			MFnDependencyNode fnJoint	= jointPath.node();
			bone->id					= ++lastJointID;
			bone->name					= xr_strdup(fnJoint.name().asChar());
			bone->path					= jointPath;

			// Add this joint to our joint map so we can look it up by name
			// cout << "Added bone '" << bone->m_name << "' to bone map\n";
			m_jointMap.insert			( NameIntMap::value_type(bone->name, bone->id) );

			// Pop one up the path stack to get our parent
			jointPath.pop(1);

			MFnDagNode parentNode(jointPath, &status);
			if ( !status ) 
			{
				status.perror("MFnDagNode constructor");
				return status;
			}
			else 
			{
				char *parentName = (char *)parentNode.name().asChar();
				NameIntMap::iterator mapIt;
				mapIt = m_jointMap.find(parentName);
				if (mapIt != m_jointMap.end()) {
					bone->parentId = (*mapIt).second;
				}
			}	

			m_boneList.push_back(bone);
		}
	}


	return MStatus::kSuccess;
}

static IC void ParseMatrix	(MTransformationMatrix& mat, Fvector& t, Fvector& r, bool bRoot)
{
	MEulerRotation rot;
	MTransformationMatrix::RotationOrder ro=MTransformationMatrix::kZXY;
	mat.reorderRotation(ro);
	rot = mat.eulerRotation();

	r.set(-(float)rot.x,-(float)rot.y,(float)rot.z);

	MVector trans;
	trans = mat.translation(MSpace::kTransform);

	MDistance dst_x(trans.x);
	MDistance dst_y(trans.y);
	MDistance dst_z(trans.z);

	t.set((float)dst_x.asMeters(),(float)dst_y.asMeters(),-(float)dst_z.asMeters());
}

IC MMatrix CalculateFullTransform(MFnTransform node)
{
	MStatus status;
	MMatrix mat = node.transformationMatrix(&status);
	int pcnt=node.parentCount(); 
	R_ASSERT3(pcnt<=1,"Joint has more than 1 parent: ",node.name().asChar());
	if (1==pcnt){
		MObject obj=node.parent(0);
		if (obj.hasFn(MFn::kTransform)){
			mat *= CalculateFullTransform(obj);
		}
	}
	return mat;
}

// this function goes through the list of bones for this model and updates
// the bone positions and rotations for the current animation frame
// note: the "m_rgInfs" array must already be set up with the DAG path to each bone
MStatus CXRaySkinExport::getBoneData (const MMatrix& locator)
{
	MMatrix locator_i = locator.inverse();
	MStatus status;

	status = gotoBindPose();

	if (status==MS::kSuccess)
	{
		for (SmdBoneIt itBones = m_boneList.begin(); itBones != m_boneList.end(); itBones++)
		{
			MFnTransform fnJoint		= (*itBones)->path.node(&status);
			LPCSTR nm					= fnJoint.name().asChar();
			if (!status) 
			{
				Msg("!Unable to cast as Transform!");
				return status;
			}

			// get rotation/translation from the transformation matrix
			MTransformationMatrix mat = fnJoint.transformationMatrix(&status); 
			if (!status) {
				Msg("!Unable to get transformation matrix!");
				return status;
			}

			if ((*itBones)->parentId==-1)
			{
				MMatrix FT			= CalculateFullTransform(fnJoint);
				MMatrix				FT2; 
				FT2.setToProduct	(FT, locator_i);
				mat					= FT2;
			}

			ParseMatrix(mat,(*itBones)->trans,(*itBones)->orient,(*itBones)->parentId==-1);

		}
	}

	return status;	
}

// export all objects parented to bones
// this is designed for mechanical objects where we simply want to parent
// geometry to bones
MStatus CXRaySkinExport::parseBoneGeometry( )
// we want to iterate through all bones
// for each bone, find list of all meshes attached to it
// for each mesh, export geometry
//
{
	MStatus status;

	// goto bind pose
	status = gotoBindPose();

	if (MStatus::kSuccess==status)
	{
		for (SmdBoneIt itBones = m_boneList.begin(); itBones != m_boneList.end(); itBones++)
		{
			MFnDagNode fnJoint = (*itBones)->path.node(&status);
			// go through children looking for geometry
			for (unsigned int i=0; i < fnJoint.childCount (&status); i++) 
			{
				MObject obj = fnJoint.child(i, &status);
				if (status == MStatus::kFailure)
				{
					status.perror("Unable to load child for bone");
					return (MStatus::kFailure);
				}
				MFnDagNode fnNode (obj);
				MDagPath path;
				status = fnNode.getPath (path);
				if (status == MStatus::kFailure)
				{
					status.perror ("unable to lookup path for child of bone");
					return (MStatus::kFailure);
				}


				// Have to make the path include the shape below it so that
				// we can determine if the underlying shape node is instanced.
				// By default, dag paths only include transform nodes.
				//
				status = path.extendToShape();
				if (status != MStatus::kSuccess) {
					// no geometry under this node...
					continue;
				}

				// If the shape is instanced then we need to determine which
				// instance this path refers to.
				//
				int instanceNum = 0;
				if (path.isInstanced())
					instanceNum = path.instanceNumber();

				MFnMesh fnMesh(path, &status);
				if (status != MStatus::kSuccess) {
					// this object is not a mesh
					continue;
				}

				// cout << "processing mesh " << fnMesh.name().asChar() << "\n";

				// Get a list of all shaders attached to this mesh

				MObjectArray rgShaders;
				MIntArray rgFaces;
				status = fnMesh.getConnectedShaders (instanceNum, rgShaders, rgFaces);
				if (status == MStatus::kFailure)
				{
					Msg("!Unable to load shaders for mesh");
					return (MStatus::kFailure);
				}

				buildEdgeTable(path);

				// now iterate through all polygons and set up that data
				MItMeshPolygon piter(path, MObject::kNullObj, &status);
				parsePolySet(piter, rgShaders, rgFaces, (*itBones)->id);

				destroyEdgeTable(); // Free up the edge table
			}
		}
	}
	return status;
}


MStatus CXRaySkinExport::parseShape( MDagPath path)
//
//  Description:
//      Find the texture files that apply to the color of each polygon of
//      a selected shape if the shape has its polygons organized into sets.
//
{

	// cout << "processing mesh " << path.partialPathName().asChar() << "\n";
	MStatus status;

	// Have to make the path include the shape below it so that
	// we can determine if the underlying shape node is instanced.
	// By default, dag paths only include transform nodes.
	//
	path.extendToShape();

	// If the shape is instanced then we need to determine which
	// instance this path refers to.
	//
	int instanceNum = 0;
	if (path.isInstanced())
		instanceNum = path.instanceNumber();

	MFnMesh fnMesh(path);

	// Get a list of all shaders attached to this mesh
	
	MObjectArray rgShaders;
	MIntArray rgFaces;
	status = fnMesh.getConnectedShaders (instanceNum, rgShaders, rgFaces);
	if (status == MStatus::kFailure)
	{
		status.perror("Unable to load shaders for mesh");
		return (MStatus::kFailure);
	}

	buildEdgeTable(path);

	// now iterate through all polygons and set up that data
	MItMeshPolygon piter(path, MObject::kNullObj, &status);
	parsePolySet(piter, rgShaders, rgFaces);

	destroyEdgeTable(); // Free up the edge table

	return MStatus::kSuccess;
}

// reset all data tables in between function calls
void CXRaySkinExport::clearData(void)
{
	for (SmdTriIt  t_it=m_triList.begin();  t_it!=m_triList.end();  t_it++)	
	{
		xr_delete(*t_it);
	}
	
	for (SmdVertIt v_it=m_vertList.begin(); v_it!=m_vertList.end(); v_it++) 
	{
		xr_delete(*v_it);
	}
	
	for (SmdBoneIt b_it=m_boneList.begin(); b_it!=m_boneList.end(); b_it++) 
	{
		xr_delete(*b_it);
	}
	xr_delete		(m_rgWeights);
	xr_free			(polySmoothingGroups);

	m_triList.clear	();
	m_vertList.clear();
	m_jointMap.clear();
	m_boneList.clear();
	m_rgInfs.clear	();
	m_strSkinCluster.clear();
}

void   CXRaySkinExport::CreateSMGFacegroups		( MFnMesh& fnMesh )
{
		// Now create a polyId->smoothingGroup table
	//   
	int numPolygons = fnMesh.numPolygons();
	polySmoothingGroups = xr_alloc<int>(numPolygons);	//(int*)malloc( sizeof(int) *  numPolygons );
	for ( int i=0; i< numPolygons; i++ ) {
		polySmoothingGroups[i] = NO_SMOOTHING_GROUP;
	}    

	// Now call the smoothingAlgorithm to fill in the polySmoothingGroups
	// table.
	// Note: we have to traverse ALL polygons to handle the case
	// of disjoint polygons.
	//
	nextSmoothingGroup = 1;
	currSmoothingGroup = 1;
	for ( int pid=0; pid<numPolygons; pid++ ) {
		newSmoothingGroup = true;
		// Check polygon has not already been visited
		if ( NO_SMOOTHING_GROUP == polySmoothingGroups[pid] ) {
			if ( !smoothingAlgorithm(pid,fnMesh) ) {
				// No smooth edges for this polygon so we set
				// the smoothing group to NO_SMOOTHING_GROUP (off)
				polySmoothingGroups[pid] = NO_SMOOTHING_GROUP;
			}
		}
	}
}
void    CXRaySkinExport::CreateSMGEdgeAttrs		( MFnMesh& fnMesh )
{

}
void  CXRaySkinExport:: CreateSmoothingGroups	( MFnMesh& fnMesh )
{
	CreateSMGEdgeAttrs	( fnMesh );
	CreateSMGFacegroups ( fnMesh );
}

	
//-----------------------------------------------------------------------------------------------
void CXRaySkinExport::buildEdgeTable( MDagPath& mesh )
{
	//.    if ( !smoothing ) return;

	// Create our edge lookup table and initialize all entries to NULL
	//
	MFnMesh fnMesh( mesh );
	edgeTableSize = fnMesh.numVertices();
	edgeTable = xr_alloc<SXREdgeInfoPtr>(edgeTableSize);
	ZeroMemory(edgeTable,edgeTableSize*sizeof(int));

	// Add entries, for each edge, to the lookup table
	//
	MItMeshEdge eIt( mesh );
	for ( ; !eIt.isDone(); eIt.next() )
	{
		bool smooth = eIt.isSmooth();
		addEdgeInfo( eIt.index(0), eIt.index(1), smooth );
	}

	// Fill in referenced polygons
	//
	MItMeshPolygon pIt( mesh );
	for ( ; !pIt.isDone(); pIt.next() )
	{
		int pvc = pIt.polygonVertexCount();
		for ( int v=0; v<pvc; v++ )
		{
			int a = pIt.vertexIndex( v );
			int b = pIt.vertexIndex( v==(pvc-1) ? 0 : v+1 );

			SXREdgeInfoPtr elem = findEdgeInfo( a, b );
			if ( NULL != elem ) {
				int edgeId = pIt.index();

				if ( INVALID_ID == elem->polyIds[0] ) {
					elem->polyIds[0] = edgeId;
				}
				else {
					elem->polyIds[1] = edgeId;
				}                

			}
		}
	}

	CreateSmoothingGroups	( fnMesh );
}

bool CXRaySkinExport::smoothingAlgorithm( int polyId, MFnMesh& fnMesh )
{
	MIntArray vertexList;
	fnMesh.getPolygonVertices( polyId, vertexList );
	int vcount = vertexList.length();
	bool smoothEdgeFound = false;

	for ( int vid=0; vid<vcount;vid++ ) {
		int a = vertexList[vid];
		int b = vertexList[ vid==(vcount-1) ? 0 : vid+1 ];

		SXREdgeInfoPtr elem = findEdgeInfo( a, b );
		if ( NULL != elem ) {
			// NOTE: We assume there are at most 2 polygons per edge
			//       halfEdge polygons get a smoothing group of
			//       NO_SMOOTHING_GROUP which is equivalent to "s off"
			//
			if ( NO_SMOOTHING_GROUP != elem->polyIds[1] ) { // Edge not a border

				// We are starting a new smoothing group
				//                
				if ( newSmoothingGroup ) {
					currSmoothingGroup = nextSmoothingGroup++;
					newSmoothingGroup = false;

					// This is a SEED (starting) polygon and so we always
					// give it the new smoothing group id.
					// Even if all edges are hard this must be done so
					// that we know we have visited the polygon.
					//
					polySmoothingGroups[polyId] = currSmoothingGroup;
				}

				// If we have a smooth edge then this poly must be a member
				// of the current smoothing group.
				//
				if ( elem->smooth ) {
					polySmoothingGroups[polyId] = currSmoothingGroup;
					smoothEdgeFound = true;
				}
				else { // Hard edge so ignore this polygon
					continue;
				}

				// Find the adjacent poly id
				//
				int adjPoly = elem->polyIds[0];
				if ( adjPoly == polyId ) {
					adjPoly = elem->polyIds[1];
				}                             

				// If we are this far then adjacent poly belongs in this
				// smoothing group.
				// If the adjacent polygon's smoothing group is not
				// NO_SMOOTHING_GROUP then it has already been visited
				// so we ignore it.
				//
				if ( NO_SMOOTHING_GROUP == polySmoothingGroups[adjPoly] ) {
					smoothingAlgorithm( adjPoly, fnMesh );
				}
				else if ( polySmoothingGroups[adjPoly] != currSmoothingGroup ) {
					Msg("!smoothing group problem at polygon %d",adjPoly);
				}
			}
		}
	}
	return smoothEdgeFound;
}

//////////////////////////////////////////////////////////////

void CXRaySkinExport::addEdgeInfo( int v1, int v2, bool smooth )
//
// Adds a new edge info element to the vertex table.
//
{
	SXREdgeInfoPtr element = NULL;

	if ( NULL == edgeTable[v1] ) {
		edgeTable[v1] = xr_alloc<SXREdgeInfo>(1);//(EdgeInfoPtr)malloc( sizeof(struct EdgeInfo) );
		element = edgeTable[v1];
	}
	else {
		element = edgeTable[v1];
		while ( NULL != element->next ) {
			element = element->next;
		}
		element->next = xr_alloc<SXREdgeInfo>(1);//(EdgeInfoPtr)malloc( sizeof(struct EdgeInfo) );
		element = element->next;
	}

	// Setup data for new edge
	//
	element->vertId     = v2;
	element->smooth     = smooth;
	element->next       = NULL;

	// Initialize array of id's of polygons that reference this edge.
	// There are at most 2 polygons per edge.
	//
	element->polyIds[0] = INVALID_ID;
	element->polyIds[1] = INVALID_ID;
}

//////////////////////////////////////////////////////////////

SXREdgeInfoPtr CXRaySkinExport::findEdgeInfo( int v1, int v2 )
//
// Finds the info for the specified edge.
//
{
	SXREdgeInfoPtr element = NULL;
	element = edgeTable[v1];

	while ( NULL != element ) {
		if ( v2 == element->vertId ) {
			return element;
		}
		element = element->next;
	}

	if ( element == NULL ) {
		element = edgeTable[v2];

		while ( NULL != element ) {
			if ( v1 == element->vertId ) {
				return element;
			}
			element = element->next;
		}
	}

	return NULL;
}

//////////////////////////////////////////////////////////////

void CXRaySkinExport::destroyEdgeTable()
//
// Free up all of the memory used by the edgeTable.
//
{
    SXREdgeInfoPtr element = NULL;
    SXREdgeInfoPtr tmp = NULL;

    for ( int v=0; v<edgeTableSize; v++ )
    {
        element = edgeTable[v];
        while ( NULL != element )
        {
            tmp = element;
            element = element->next;
            xr_free( tmp );
        }
    }

    if ( NULL != edgeTable ) {
        xr_free( edgeTable );
        edgeTable = NULL;
    }
    
    if ( NULL != polySmoothingGroups ) {
        xr_free( polySmoothingGroups );
        polySmoothingGroups = NULL;
    }
}
//////////////////////////////////////////////////////////////
