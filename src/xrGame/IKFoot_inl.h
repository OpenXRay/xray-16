
IC	Fvector& CIKFoot::ToePosition( Fvector &toe_position ) const
{
	
	//toe_position.set( m_toe_position );
	//return toe_position;
	return	get_local_vector( toe_position, m_toe_position );
}
IC	Fvector& CIKFoot::HeelPosition( Fvector &heel_position ) const
{
	heel_position.set( m_heel_position.v );
	return heel_position;
	//return	get_local_vector( heel_position,  m_heel_position );
}
IC	Fvector& CIKFoot::FootNormal( Fvector &foot_normal ) const
{
	return	get_local_vector(foot_normal,  m_foot_normal );
}
IC	Fvector&	CIKFoot::get_local_vector(  Fvector &v, const local_vector &lv )const
{
	return	get_local_vector( ref_bone(), v, lv );
}
IC	Fvector&	CIKFoot::get_local_vector( u16 bone, Fvector &v, const local_vector &lv )const
{
	
	if( bone == lv.bone )
	{
		v.set(lv.v);
	} 
	else if ( bone == 2 && lv.bone == 3 )
	{
		m_bind_b2_to_b3.transform_tiny( v, lv.v );
	} 
	else if ( bone == 3 && lv.bone == 2 )
	{
		Fmatrix().invert( m_bind_b2_to_b3 ).transform_tiny( v, lv.v );
	}
	else 
		VERIFY( 0 );

	return v;

	//switch( 1 + ref_bone() - lv.bone )
	//{
	//	case 0:	 
	//	case 1:	
	//}

}



