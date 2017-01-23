#ifndef	_SMOTH_FLAGS_H_
#define	_SMOTH_FLAGS_H_

IC void set_face_adge_hard ( u32 &flags, u16 edge_idx )
{
	VERIFY( edge_idx<3 );
	flags |= ( 1<< (4-edge_idx)%3 );
}

template< typename export_class >
MStatus t_set_smoth_flags( export_class& export_obj, u32 &flags, const MIntArray& tri_vert_indeces )
{
	if( tri_vert_indeces.length()!=3 )
	{
		Msg("XRAY Plagin ERROR");
		return MS::kFailure;
	}
	flags = u32(0);
	for( u16 i = 0; 3 > i; ++i )
	{
		int a = tri_vert_indeces[ i ];
		int b = tri_vert_indeces[ (i+1)%3 ];
		SXREdgeInfoPtr elem = export_obj.findEdgeInfo( a, b );

		if( elem && !elem->smooth )
			set_face_adge_hard( flags, i );
	}

	return MS::kSuccess;
}

#endif