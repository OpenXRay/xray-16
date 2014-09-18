#ifndef	_FACE_SMOTH_FLAGS_H_
#define	_FACE_SMOTH_FLAGS_H_
enum
{
	backface_flag = 1<<3
};

IC bool is_backface( u32 face_flags )
{
	return !!( face_flags & backface_flag );
}

IC void set_backface( u32 &face_flags, bool value )
{
	if( value )
			face_flags|=backface_flag;
	else
			face_flags&=~backface_flag;
}

IC u16 convert_edge_index( u32 face_flags, u16 edge_index )
{
	VERIFY( edge_index>=0 && edge_index<=2 );
	if( is_backface( face_flags ) )
		return (4 - edge_index)%3;
	else
		return edge_index;
}

IC bool is_soft_edge( u32 face_flags, u16 edge_index )
{
	edge_index = convert_edge_index( face_flags, edge_index );
	VERIFY( edge_index>=0 && edge_index<=2 );
	return  !( face_flags & ( 1<<edge_index ) );
}

IC void set_soft_edge( u32 &face_flags, u16 edge_index, bool value )
{
	edge_index = convert_edge_index( face_flags, edge_index );
	VERIFY( edge_index>=0 && edge_index<=2 );
	if(value)
		face_flags &=	~(1<<edge_index);
	else
		face_flags |=	(1<<edge_index);
}


IC  bool do_connect_faces_by_faces_edge_flags( u32 start_face_flags, u32 test_face_flags, u16 start_common_edge_idx, u16 test_common_edge_idx )
	{
		bool start_edge_back = is_backface( start_face_flags );
		bool test_edge_back = is_backface( test_face_flags );
		if(start_edge_back != test_edge_back)
			return false;

		bool start_edge_smooth = is_soft_edge( start_face_flags, start_common_edge_idx );//!( start.sm_group & (1<<start_common_face) );
		bool test_edge_smooth  = is_soft_edge( test_face_flags, test_common_edge_idx );//!( test.sm_group  & (1<<test_common_face) );
		return start_edge_smooth && test_edge_smooth;
	}

#endif