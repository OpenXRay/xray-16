//----------------------------------------------------------------------------//
// Helper.h                                                                   //
// Copyright (C) 2001 Bruno 'Beosil' Heidelberger                             //
//----------------------------------------------------------------------------//
// This program is free software; you can redistribute it and/or modify it    //
// under the terms of the GNU General Public License as published by the Free //
// Software Foundation; either version 2 of the License, or (at your option)  //
// any later version.                                                         //
//----------------------------------------------------------------------------//

#ifndef HELPER_H
#define HELPER_H

//----------------------------------------------------------------------------//
// Defines                                                                    //
//----------------------------------------------------------------------------//

// this is the class for all biped controllers except the root and the footsteps
#define BIPSLAVE_CONTROL_CLASS_ID Class_ID(0x9154,0)
// this is the class for the center of mass, biped root controller ("Bip01")
#define BIPBODY_CONTROL_CLASS_ID  Class_ID(0x9156,0) 

//----------------------------------------------------------------------------//
// Class declaration                                                          //
//----------------------------------------------------------------------------//

class Helper
{
// constructors/destructor
protected:
					Helper			();
	virtual			~Helper			();

// member functions
public:
	static Matrix3	GetBoneTM		(INode *pNode, TimeValue t);
	static BOOL		IsBipedBone		(INode *pNode);
	static BOOL		IsBone			(INode *pNode, BOOL bAllowDummy);
	static BOOL		IsMesh			(INode *pNode);
	static void		SetBipedUniform	(INode *pNode, BOOL bUniform, BOOL bFigure);
	static IC string ConvertSpace	(string input)
	{
		string result = "";
		for (DWORD i=0; i<input.size(); i++)
			result += (input[i]==' ')?'_':input[i];
		char * res_ptr = (char *)result.c_str();
		_strlwr(res_ptr);
		return result;
	}
	static IC void ConvertMatrix	(const Matrix3& _src, Fmatrix& dest)
	{
//*		
		Fmatrix src; src.identity();
		Point3 cn,rw;
		cn = _src.GetRow(0);
		src.i.set(cn.x,cn.y,cn.z);
		cn = _src.GetRow(1);
		src.j.set(cn.x,cn.y,cn.z);
		cn = _src.GetRow(2);
		src.k.set(cn.x,cn.y,cn.z);
		rw = _src.GetRow(3);
		src.c.set(rw.x,rw.y,rw.z);

		Fmatrix m,t;
		m.identity	();
		m.i.set(1,0,0);
		m.j.set(0,0,1);
		m.k.set(0,1,0);

		t.mul		(m,src);
		dest.mul	(t,m);
/*/
/*
		Quat	Q;
		Point3	P,S;
		DecomposeMatrix(_src, P, Q, S);
		Fquaternion q;
		q.set(Q.w,-Q.x,-Q.z,-Q.y);
		dest.rotation(q);
		dest.translate_over(P.x,P.z,P.y);
//*/
	}
};

#endif

//----------------------------------------------------------------------------//
