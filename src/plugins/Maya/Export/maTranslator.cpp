#include "stdafx.h"
#pragma hdrstop

#include "maTranslator.h"
#include "..\..\..\editors\Ecore\editor\EditObject.h"

#define NO_SMOOTHING_GROUP      -1
#define INITIALIZE_SMOOTHING    -2
#define INVALID_ID              -1

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void* CXRayObjectExport::creator()
{
	return new CXRayObjectExport();
}

//////////////////////////////////////////////////////////////

MStatus CXRayObjectExport::reader ( const MFileObject& file, const MString& options, FileAccessMode mode)
{
    fprintf(stderr, "CXRayObjectExport::reader called in error\n");
    return MS::kFailure;
}
//////////////////////////////////////////////////////////////

MStatus CXRayObjectExport::writer ( const MFileObject& file, const MString& options, FileAccessMode mode )
{
	MStatus status= MS::kFailure;
	int export_mode = 0;
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

            if( theOption[0] == MString("ogf") ) 
			{
				if(theOption.length() > 1)
					export_mode = theOption[1].asInt();
			}
        }
	}
	Log							("export_mode: ",export_mode);


	//move default extesion here..
	MString mname				= file.fullName()+".object";
	LPCSTR	fname				= mname.asChar();

	Log							("Export object: ",fname);
	CEditableObject* OBJECT		= xr_new<CEditableObject>(fname);
	OBJECT->SetVersionToCurrent	(TRUE,TRUE);
	
	if((mode==MPxFileTranslator::kExportAccessMode)||(mode==MPxFileTranslator::kSaveAccessMode))
	{
		status = ExportAll(OBJECT)?MS::kSuccess:MS::kFailure;
	}else 
	if(mode==MPxFileTranslator::kExportActiveAccessMode)
	{
		status = ExportSelected(OBJECT)?MS::kSuccess:MS::kFailure;
	}
	if (MS::kSuccess==status)
	{ 
		OBJECT->Optimize		();
		OBJECT->Save			(fname);

		Log						("Object succesfully exported.");
		Msg						("%d vertices, %d faces", OBJECT->GetVertexCount(), OBJECT->GetFaceCount(true, false));
		if(export_mode)
		{
			mname					= file.fullName()+".ogf";
			Log						("Export OGF object: ",fname);
			fname					= mname.asChar();
			
			OBJECT->m_objectFlags.set(CEditableObject::eoProgressive, (export_mode==1) );
			

			OBJECT->ExportOGF		(fname, 4);
			Log						("OGF object succesfully exported.");
		}
	}else
	{
		Log("!Export failed.");
	}
	xr_delete(OBJECT);

	return status;
}
//////////////////////////////////////////////////////////////

MString CXRayObjectExport::filter()const
{
	std::cerr <<"Dbg : MString CXRayObjectExport::filter()const\n";
	return "*.ob*";
}

//////////////////////////////////////////////////////////////

bool CXRayObjectExport::haveReadMethod () const
{
    return false;
}
//////////////////////////////////////////////////////////////

bool CXRayObjectExport::haveWriteMethod () const
{
	std::cerr <<"Dbg : bool CXRayObjectExport::haveWriteMethod () const\n";
    return true;
}
//////////////////////////////////////////////////////////////
/*
MString CXRayObjectExport::defaultExtension () const
{
	std::cerr <<"Dbg : MString CXRayObjectExport::defaultExtension () const\n";
	return "object";
}
*/
//////////////////////////////////////////////////////////////

MPxFileTranslator::MFileKind CXRayObjectExport::identifyFile (
                                        const MFileObject& fileName,
                                        const char* buffer,
                                        short size) const
{
    const char * name = fileName.name().asChar();
    int   nameLength = xr_strlen(name);
    
    //if ((nameLength > 7) && !stricmp(name+nameLength-7, ".object"))
	if ((nameLength > 4) && !stricmp(name+nameLength-4, ".xro"))
        return kCouldBeMyFileType;
    else
        return kNotMyFileType;
}
//////////////////////////////////////////////////////////////

bool CXRayObjectExport::initializeSetsAndLookupTables( bool exportAll )
//
// Description :
//    Creates a list of all sets in Maya, a list of mesh objects,
//    and polygon/vertex lookup tables that will be used to
//    determine which sets are referenced by the poly components.
//
{
	int i=0,j=0, length;
	MStatus stat;
	
	// Initialize class data.
	// Note: we cannot do this in the constructor as it
	// only gets called upon registry of the plug-in.
	//
	numSets = 0;
	sets = NULL;
	lastSets = NULL;
	lastMaterials = NULL;
	objectId = 0;
	objectCount = 0;
	polygonTable = NULL;
	vertexTable = NULL;
	polygonTablePtr = NULL;
	vertexTablePtr = NULL;
	objectGroupsTablePtr = NULL;
	objectNodeNamesArray.clear();
	transformNodeNameArray.clear();

	//////////////////////////////////////////////////////////////////
	//
	// Find all sets in Maya and store the ones we care about in
	// the 'sets' array. Also make note of the number of sets.
	//
	//////////////////////////////////////////////////////////////////
	
	// Get all of the sets in maya and put them into
	// a selection list
	// 
	MStringArray result;
	MGlobal::executeCommand( "ls -sets", result );
	MSelectionList * setList = xr_new<MSelectionList>();
	length = result.length();
	for ( i=0; i<length; i++ )
	{	
		setList->add( result[i] );
	}
	
	// Extract each set as an MObject and add them to the
	// sets array.
	// We may be excluding groups, matierials, or ptGroups
	// in which case we can ignore those sets. 
	//
	MObject mset;
	sets = xr_new<MObjectArray>();
	length = setList->length();
	for ( i=0; i<length; i++ )
	{
		setList->getDependNode( i, mset );
		
		MFnSet fnSet( mset, &stat );
		if ( stat ) {
			if ( MFnSet::kRenderableOnly == fnSet.restriction(&stat) ) {
				sets->append( mset );
			}
		}	
	}
	xr_delete(setList);
	
	numSets = sets->length();
			
	//////////////////////////////////////////////////////////////////
	//
	// Do a dag-iteration and for every mesh found, create facet and
	// vertex look-up tables. These tables will keep track of which
	// sets each component belongs to.
	//
	// If exportAll is false then iterate over the activeSelection 
	// list instead of the entire DAG.
	//
	// These arrays have a corrisponding entry in the name
	// stringArray.
	//
	//////////////////////////////////////////////////////////////////
	MIntArray vertexCounts;
	MIntArray polygonCounts;	
			
	if ( exportAll ) {
		MItDag dagIterator( MItDag::kBreadthFirst, MFn::kInvalid, &stat);

    	if ( MS::kSuccess != stat) {
    	    fprintf(stderr,"Failure in DAG iterator setup.\n");
    	    return false;
    	}
		
		objectNames = xr_new<MStringArray>();
		
    	for ( ; !dagIterator.isDone(); dagIterator.next() ) 
		{
    	    MDagPath dagPath;
    	    stat = dagIterator.getPath( dagPath );

			if ( stat ) 
			{
				// skip over intermediate objects
				//
				MFnDagNode dagNode( dagPath, &stat );
				if (dagNode.isIntermediateObject()) 
				{
					continue;
				}

				if (( dagPath.hasFn(MFn::kMesh)) &&
					( dagPath.hasFn(MFn::kTransform)))
				{
					// We want only the shape, 
					// not the transform-extended-to-shape.
					continue;
				}
				else if ( dagPath.hasFn(MFn::kMesh))
				{
					// We have a mesh so create a vertex and polygon table
					// for this object.
					//
					MFnMesh fnMesh( dagPath );
					int vtxCount = fnMesh.numVertices();
					int polygonCount = fnMesh.numPolygons();
					// we do not need this call anymore, we have the shape.
					// dagPath.extendToShape();
					MString name = dagPath.fullPathName();
					objectNames->append( name );
					objectNodeNamesArray.append( fnMesh.name() );

					vertexCounts.append( vtxCount );
					polygonCounts.append( polygonCount );

					objectCount++;
				}
			}
		}	
	}else{
		MSelectionList slist;
    	MGlobal::getActiveSelectionList( slist );
    	MItSelectionList iter( slist );
		MStatus status;

		objectNames = xr_new<MStringArray>();

		// We will need to interate over a selected node's heirarchy
		// in the case where shapes are grouped, and the group is selected.
		MItDag dagIterator( MItDag::kDepthFirst, MFn::kInvalid, &status);

    	for ( ; !iter.isDone(); iter.next() ){
			MDagPath objectPath;
			stat = iter.getDagPath( objectPath );

			// reset iterator's root node to be the selected node.
			status = dagIterator.reset (objectPath.node(), 
										MItDag::kDepthFirst, MFn::kInvalid );

			// DAG iteration beginning at at selected node
			for ( ; !dagIterator.isDone(); dagIterator.next() ){
				MDagPath dagPath;
				MObject  component = MObject::kNullObj;
				status = dagIterator.getPath(dagPath);

				if (!status){
					fprintf(stderr,"Failure getting DAG path.\n");
					freeLookupTables();
					return false;
				}

                // skip over intermediate objects
                //
                MFnDagNode dagNode( dagPath, &stat );
                if (dagNode.isIntermediateObject()) continue;

				if (( dagPath.hasFn(MFn::kMesh)) && ( dagPath.hasFn(MFn::kTransform))){
					// We want only the shape, 
					// not the transform-extended-to-shape.
					continue;
				}else if ( dagPath.hasFn(MFn::kMesh)){
					// We have a mesh so create a vertex and polygon table
					// for this object.
					//
					MFnMesh fnMesh( dagPath );
					int vtxCount = fnMesh.numVertices();
					int polygonCount = fnMesh.numPolygons();

					// we do not need this call anymore, we have the shape.
					// dagPath.extendToShape();
					MString name = dagPath.fullPathName();
					objectNames->append( name );
					objectNodeNamesArray.append( fnMesh.name() );
									
					vertexCounts.append( vtxCount );
					polygonCounts.append( polygonCount );

					objectCount++;	
				}
    		}
		}
	}

	// Now we know how many objects we are dealing with 
	// and we have counts of the vertices/polygons for each
	// object so create the maya group look-up table.
	//
	if( objectCount > 0 ) {
		// To export Maya groups we traverse the hierarchy starting at
		// each objectNodeNamesArray[i] going towards the root collecting transform
		// nodes as we go.
		length = objectNodeNamesArray.length();
		for( i=0; i<length; i++ ) {
			MIntArray transformNodeNameIndicesArray;
			recFindTransformDAGNodes( objectNodeNamesArray[i], transformNodeNameIndicesArray );
		}

		if( transformNodeNameArray.length() > 0 ) {
			objectGroupsTablePtr = xr_alloc<bool*>(objectCount);// (bool**) malloc( sizeof(bool*)*objectCount );
			length = transformNodeNameArray.length();
			for ( i=0; i<objectCount; i++ )
			{
//				objectGroupsTablePtr[i] = (bool*)calloc( length, sizeof(bool) );	
				objectGroupsTablePtr[i] = xr_alloc<bool>(length);
				ZeroMemory(objectGroupsTablePtr[i],length*sizeof(bool));
				
				if ( objectGroupsTablePtr[i] == NULL ) {
					Log("!calloc returned NULL (objectGroupsTablePtr)");
					return false;
				}
			}
		}
//		else{
//			Log("!Can't find transform for node.");
//			return false;
//		}
	}

	// Create the vertex/polygon look-up tables.
	//
	if ( objectCount > 0 ) {
		
		vertexTablePtr = xr_alloc<bool*>(objectCount);	//(bool**) malloc( sizeof(bool*)*objectCount );
		polygonTablePtr = xr_alloc<bool*>(objectCount);	//(bool**) malloc( sizeof(bool*)*objectCount );
	
		for ( i=0; i<objectCount; i++ )
		{
//			vertexTablePtr[i] = (bool*)calloc( vertexCounts[i]*numSets, sizeof(bool) );	
			vertexTablePtr[i] = xr_alloc<bool>(vertexCounts[i]*numSets);
			ZeroMemory(vertexTablePtr[i],vertexCounts[i]*numSets*sizeof(bool));

			if ( vertexTablePtr[i] == NULL ) {
				Log("!calloc returned NULL (vertexTable)");
				return false;
			}
	
//			polygonTablePtr[i] = (bool*)calloc( polygonCounts[i]*numSets, sizeof(bool) );
			polygonTablePtr[i] = xr_alloc<bool>(polygonCounts[i]*numSets);
			ZeroMemory(polygonTablePtr[i],polygonCounts[i]*numSets*sizeof(bool));
			if ( polygonTablePtr[i] == NULL ) {
				Log("!calloc returned NULL (polygonTable)");
				return false;
			}
		}	
	}

	// If we found no meshes then return
	//	
	if ( objectCount == 0 ) {
		return false;
	}
	
	//////////////////////////////////////////////////////////////////
	//
	// Go through all of the set members (flattened lists) and mark
	// in the lookup-tables, the sets that each mesh component belongs
	// to.
	//
	//
	//////////////////////////////////////////////////////////////////
	bool flattenedList = true;
	MDagPath object;
	MObject component;
	MSelectionList memberList;
	
	
	for ( i=0; i<numSets; i++ )
	{
		MFnSet fnSet( (*sets)[i] );		
		memberList.clear();
		stat = fnSet.getMembers( memberList, flattenedList );

		if (MS::kSuccess != stat) {
			fprintf(stderr,"Error in fnSet.getMembers()!\n");
		}

		int m, numMembers;
		numMembers = memberList.length();
		for ( m=0; m<numMembers; m++ )
		{
			if ( memberList.getDagPath(m,object,component) ) {

				if ( (!component.isNull()) && (object.apiType() == MFn::kMesh) )
				{
					if (component.apiType() == MFn::kMeshVertComponent) {
						MItMeshVertex viter( object, component );	
						for ( ; !viter.isDone(); viter.next() )
						{
							int compIdx = viter.index();
							MString name = object.fullPathName();
							
							// Figure out which object vertexTable
							// to get.
							//

							int o, numObjectNames;
							numObjectNames = objectNames->length();
							for ( o=0; o<numObjectNames; o++ ) {
								if ( (*objectNames)[o] == name ) {
									// Mark set i as true in the table
									//		
									vertexTable = vertexTablePtr[o];
									*(vertexTable + numSets*compIdx + i) = true;
									break;
								}
							}
						}
					}
					else if (component.apiType() == MFn::kMeshPolygonComponent) 
					{
						MItMeshPolygon piter( object, component );
						for ( ; !piter.isDone(); piter.next() )
						{
							int compIdx = piter.index();
							MString name = object.fullPathName();
							
							// Figure out which object polygonTable
							// to get.
							//							
							int o, numObjectNames;
							numObjectNames = objectNames->length();
							for ( o=0; o<numObjectNames; o++ ) {
								if ( (*objectNames)[o] == name ) {
									
									// Mark set i as true in the table
									//

									// Check for bad components in the set
									//									
									if ( compIdx >= polygonCounts[o] ) {
										Msg("!Bad polygon index '%d' found. Polygon skipped",compIdx);
										break;
									}
									
									polygonTable = polygonTablePtr[o];
									*(polygonTable + numSets*compIdx + i) = true;
									break;
								}
							}	
						}
					}										
				}
				else { 

				// There are no components, therefore we can mark
				// all polygons as members of the given set.
				//

				if (object.hasFn(MFn::kMesh)) {

					MFnMesh fnMesh( object, &stat );
					if ( MS::kSuccess != stat) {
						fprintf(stderr,"Failure in MFnMesh initialization.\n");
						return false;
					}

					// We are going to iterate over all the polygons.
					//
					MItMeshPolygon piter( object, MObject::kNullObj, &stat );
					if ( MS::kSuccess != stat) {
						fprintf(stderr,
								"Failure in MItMeshPolygon initialization.\n");
						return false;
					}
					for ( ; !piter.isDone(); piter.next() )
					{
						int compIdx = piter.index();
						MString name = object.fullPathName();

						// Figure out which object polygonTable to get.
						//
						int o, numObjectNames;
						numObjectNames = objectNames->length();
						for ( o=0; o<numObjectNames; o++ ) {
							if ( (*objectNames)[o] == name ) {
								// Check for bad components in the set
								//
								if ( compIdx >= polygonCounts[o] ) {
									Msg("!Bad polygon index '%d' found. Polygon skipped",compIdx);
									break;
								}
								// Mark set i as true in the table
								//
								polygonTable = polygonTablePtr[o];
								*(polygonTable + numSets*compIdx + i) = true;
								break;
							}
						}
					} // end of piter.next() loop
				} // end of condition if (object.hasFn(MFn::kMesh))
				} // end of else condifion if (!component.isNull()) 
			} // end of memberList.getDagPath(m,object,component)
		} // end of memberList loop
	} // end of for-loop for sets

	// Go through all of the group members and mark in the
	// lookup-table, the group that each shape belongs to.
	length = objectNodeNamesArray.length();
	if (objectGroupsTablePtr){
		for( i=0; i<length; i++ ) {
			MIntArray groupTableIndicesArray;
			bool *objectGroupTable = objectGroupsTablePtr[i];
			int length2;
			recFindTransformDAGNodes( objectNodeNamesArray[i], groupTableIndicesArray );
			length2 = groupTableIndicesArray.length();
			for( j=0; j<length2; j++ ) {
				int groupIdx = groupTableIndicesArray[j];
				objectGroupTable[groupIdx] = true;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////

void CXRayObjectExport::freeLookupTables()
//
// Frees up all tables and arrays allocated by this plug-in.
//
{
	if (vertexTablePtr){
		for ( int i=0; i<objectCount; i++ ) {
			if ( vertexTablePtr[i] != NULL ) {
				xr_free( vertexTablePtr[i] );
			}
		}
	}
	if (polygonTablePtr){
		for ( int i=0; i<objectCount; i++ ) {
			if ( polygonTablePtr[i] != NULL ) {
				xr_free( polygonTablePtr[i] );
			}
		}	
	}
	if (objectGroupsTablePtr) {
		for ( int i=0; i<objectCount; i++ ) {
			if ( objectGroupsTablePtr[i] != NULL ) {
				xr_free( objectGroupsTablePtr[i] );
			}
		}
		xr_free( objectGroupsTablePtr );
		objectGroupsTablePtr = NULL;
	}
	
	if ( vertexTablePtr != NULL ) {
		xr_free( vertexTablePtr );
		vertexTablePtr = NULL;
	}
	if ( polygonTablePtr != NULL ) {
		xr_free( polygonTablePtr );
		polygonTablePtr = NULL;
	}

	if ( lastSets != NULL ) {
		xr_delete(lastSets);
		lastSets = NULL;
	}
	
	if ( lastMaterials != NULL ) {
		xr_delete(lastMaterials);
		lastMaterials = NULL;
	}
	
	if ( sets != NULL ) {
		xr_delete(sets);
		sets = NULL;
	}
	
	if ( objectNames != NULL ) {
		xr_delete(objectNames);
		objectNames = NULL;
	}		
}

//////////////////////////////////////////////////////////////

bool CXRayObjectExport::lookup( MDagPath& dagPath, int setIndex, int compIdx)
{
	polygonTable = polygonTablePtr[objectId];
	bool ret = *(polygonTable + numSets*compIdx + setIndex);			
	return ret;
}	

//////////////////////////////////////////////////////////////

void CXRayObjectExport::buildEdgeTable( MDagPath& mesh )
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

	 CreateSmoothingGroups( fnMesh );
}

//////////////////////////////////////////////////////////////
void   CXRayObjectExport:: CreateSmoothingGroups	( MFnMesh& fnMesh )
{
	    // Now create a polyId->smoothingGroup table
    //   
    int numPolygons = fnMesh.numPolygons();
    polySmoothingGroups = xr_alloc<int>(numPolygons);	//(int*)malloc( sizeof(int) *  numPolygons );
    for ( int i=0; i< numPolygons; i++ ) {
        polySmoothingGroups[i] = NO_SMOOTHING_GROUP;
    }    
	//CreateSMGFacegroups( fnMesh );
	// CreateSMGEdgeAttrs( fnMesh );
}
void   CXRayObjectExport::  CreateSMGFacegroups		( MFnMesh& fnMesh )
{
	    // Now call the smoothingAlgorithm to fill in the polySmoothingGroups
    // table.
    // Note: we have to traverse ALL polygons to handle the case
    // of disjoint polygons.
    //
	int numPolygons = fnMesh.numPolygons();
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
	/*
		for ( int idx=0; idx<numPolygons; ++idx ) 
		{
			string128			buff;
			sprintf				(buff,"[%d] smg=%d",idx,polySmoothingGroups[idx]);
			Msg					(buff);
		}
	*/
}
void	set_edge_smooth_flag( int &sm_group, int edge, bool value)
{
	if( value )
	{
		sm_group &= ~( 1<< edge );
//		u32 sg = sm_group;
//		sm_group = sg;

	}
}

void   CXRayObjectExport:: CreateSMGEdgeAttrs		( MFnMesh& fnMesh )
{
	int numPolygons = fnMesh.numPolygons();
	 for ( int pid=0; pid<numPolygons; pid++ ) 
	 {
		MIntArray vertexList;
		fnMesh.getPolygonVertices( pid, vertexList );
		int vcount = vertexList.length();
		if(vcount!=3)
			Msg( "poly vertex count not equel 3(is not a tri) vertex count = %d", vcount );
		
		for ( int vid=0; vid<vcount;vid++ ) 
		{
			int a = vertexList[vid];
			int b = vertexList[ vid==(vcount-1) ? 0 : vid+1 ];

			SXREdgeInfoPtr elem = findEdgeInfo( a, b );

			if ( NULL != elem )
			{
				set_edge_smooth_flag( polySmoothingGroups[pid], vid, elem->smooth );
			}
		}
	 }
}

bool CXRayObjectExport::smoothingAlgorithm( int polyId, MFnMesh& fnMesh )
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

void CXRayObjectExport::addEdgeInfo( int v1, int v2, bool smooth )
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

SXREdgeInfoPtr CXRayObjectExport::findEdgeInfo( int v1, int v2 )
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

void CXRayObjectExport::destroyEdgeTable()
//
// Free up all of the memory used by the edgeTable.
//
{
//.    if ( !smoothing ) return;
    
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

void CXRayObjectExport::recFindTransformDAGNodes( MString& nodeName, MIntArray& transformNodeIndicesArray )
{
	// To handle Maya groups we traverse the hierarchy starting at
	// each objectNames[i] going towards the root collecting transform
	// nodes as we go.
	MStringArray result;
	MString cmdStr = "listRelatives -ap " + nodeName;
	MGlobal::executeCommand( cmdStr, result );
	
	if( result.length() == 0 )
		// nodeName must be at the root of the DAG.  Stop recursing
		return;

	for( unsigned int j=0; j<result.length(); j++ ) {
		// check if the node result[i] is of type transform
		MStringArray result2;
		MGlobal::executeCommand( "nodeType " + result[j], result2 );
		
		if( result2.length() == 1 && result2[0] == "transform" ) {
			// check if result[j] is already in result[j]
			bool found=false;
			unsigned int i;
			for( i=0; i<transformNodeNameArray.length(); i++) {
				if( transformNodeNameArray[i] == result[j] ) {
					found = true;
					break;
				}
			}

			if( !found ) {
				transformNodeIndicesArray.append(transformNodeNameArray.length());
				transformNodeNameArray.append(result[j]);
			}
			else {
				transformNodeIndicesArray.append(i);
			}
			recFindTransformDAGNodes(result[j], transformNodeIndicesArray);
		}
	}
}

