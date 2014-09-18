////////////////////////////////////////////////////////////////////////////
//	Module 		: CameraRecoil.h
//	Created 	: 26.05.2008
//	Author		: Evgeniy Sokolov
//	Description : Camera Recoil struct
////////////////////////////////////////////////////////////////////////////

#ifndef CAMERA_RECOIL_H_INCLUDED
#define CAMERA_RECOIL_H_INCLUDED

//מעהאקא ןנט סענוכüבו 
struct CameraRecoil
{
	float		RelaxSpeed;
	float		RelaxSpeed_AI;
	float		Dispersion;
	float		DispersionInc;
	float		DispersionFrac;
	float		MaxAngleVert;
	float		MaxAngleHorz;
	float		StepAngleHorz;
	bool		ReturnMode;
	bool		StopReturn;

	CameraRecoil():
		MaxAngleVert	( EPS   ),
		RelaxSpeed		( EPS_L ),
		RelaxSpeed_AI	( EPS_L ),
		Dispersion		( EPS   ),
		DispersionInc	( 0.0f  ),
		DispersionFrac	( 1.0f  ),
		MaxAngleHorz	( EPS   ),
		StepAngleHorz	( 0.0f  ),
		ReturnMode		( false ),
		StopReturn		( false )
	{};

	CameraRecoil( const CameraRecoil& clone )		{	Clone( clone );	}

	IC void Clone( const CameraRecoil& clone )
	{
		// *this = clone;
		RelaxSpeed		= clone.RelaxSpeed;
		RelaxSpeed_AI	= clone.RelaxSpeed_AI;
		Dispersion		= clone.Dispersion;
		DispersionInc	= clone.DispersionInc;
		DispersionFrac	= clone.DispersionFrac;
		MaxAngleVert	= clone.MaxAngleVert;
		MaxAngleHorz	= clone.MaxAngleHorz;
		StepAngleHorz	= clone.StepAngleHorz;

		ReturnMode		= clone.ReturnMode;
		StopReturn		= clone.StopReturn;
		
		VERIFY( !fis_zero(RelaxSpeed)    );
		VERIFY( !fis_zero(RelaxSpeed_AI) );
		VERIFY( !fis_zero(MaxAngleVert)  );
		VERIFY( !fis_zero(MaxAngleHorz)  );
	}
}; //struct CameraRecoil

#endif // CAMERA_RECOIL_H_INCLUDED
