

IC void CPHElement::InverceLocalForm(Fmatrix& m)
{
	m.identity();
	m.c.set(m_mass_center);
	m.invert();
}

IC void CPHElement::MulB43InverceLocalForm(Fmatrix& m) const
{
	Fvector ic;ic.set(m_mass_center);
	ic.invert();
	m.transform_dir(ic);
	m.c.add(ic);
}


IC		void	CPHElement::CalculateBoneTransform( Fmatrix &bone_transform )const
{
	Fmatrix parent;
	parent.invert		( m_shell->mXFORM );
	bone_transform.mul_43( parent, mXFORM );
}

IC		void		CPHElement::ActivatingPos(const Fmatrix &BoneTransform)
{
	ToBonePos( BoneTransform, mh_unspecified );
	m_flags.set( flActivating, FALSE );
	if( !m_parent_element ) 
		m_shell->SetObjVsShellTransform(BoneTransform);

	//{
	//	m_shell->m_object_in_root.set( B->mTransform );
	//	m_shell->m_object_in_root.invert( );
	//	m_shell->SetNotActivating( );
	//}
	//VERIFY2(fsimilar(DET(B->mTransform),1.f,DET_CHECK_EPS),"Bones callback returns 0 matrix");

	VERIFY_RMATRIX( BoneTransform );
	VERIFY( valid_pos( BoneTransform.c, phBoundaries ) );
	return;
}