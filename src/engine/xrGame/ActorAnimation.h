#ifndef _Actor_Animation_H
#define _Actor_Animation_H
#pragma once

// animation state constants
//-------------------------------------------------------------------------------
#define _Fwd			(mcFwd)
#define _Back			(mcBack)
#define _LStr			(mcLStrafe)
#define _RStr			(mcRStrafe)
#define _FwdLStr		(mcFwd|mcLStrafe)
#define _FwdRStr		(mcFwd|mcRStrafe)
#define _BackLStr		(mcBack|mcLStrafe)
#define _BackRStr		(mcBack|mcRStrafe)
	
#define _AFwd			(mcAccel|mcFwd)
#define _ABack			(mcAccel|mcBack)
#define _ALStr			(mcAccel|mcLStrafe)
#define _ARStr			(mcAccel|mcRStrafe)
#define _AFwdLStr		(mcAccel|mcFwd|mcLStrafe)
#define _AFwdRStr		(mcAccel|mcFwd|mcRStrafe)
#define _ABackLStr		(mcAccel|mcBack|mcLStrafe)
#define _ABackRStr		(mcAccel|mcBack|mcRStrafe)
// 
#define _Crch			(mcCrouch)
#define _ACrch			(mcCrouch|mcAccel)
#define _CrchFwd		(mcCrouch|mcFwd)
#define _CrchBack		(mcCrouch|mcBack)
#define _CrchLStr		(mcCrouch|mcLStrafe)
#define _CrchRStr		(mcCrouch|mcRStrafe)
#define _CrchFwdLStr	(mcCrouch|mcFwd|mcLStrafe)
#define _CrchFwdRStr	(mcCrouch|mcFwd|mcRStrafe)
#define _CrchBackLStr	(mcCrouch|mcBack|mcLStrafe)
#define _CrchBackRStr	(mcCrouch|mcBack|mcRStrafe)
#define _ACrchFwd		(mcCrouch|mcAccel|mcFwd)
#define _ACrchBack		(mcCrouch|mcAccel|mcBack)
#define _ACrchLStr		(mcCrouch|mcAccel|mcLStrafe)
#define _ACrchRStr		(mcCrouch|mcAccel|mcRStrafe)
#define _ACrchFwdLStr	(mcCrouch|mcAccel|mcFwd|mcLStrafe)
#define _ACrchFwdRStr	(mcCrouch|mcAccel|mcFwd|mcRStrafe)
#define _ACrchBackLStr	(mcCrouch|mcAccel|mcBack|mcLStrafe)
#define _ACrchBackRStr	(mcCrouch|mcAccel|mcBack|mcRStrafe)

#define _Jump			(mcJump)
//-------------------------------------------------------------------------------

#endif //_Actor_Animation_H
