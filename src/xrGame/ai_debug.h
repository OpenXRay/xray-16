////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_debug.h
//	Created 	: 02.10.2001
//  Modified 	: 11.11.2003
//	Author		: Oles Shihkovtsov, Dmitriy Iassenev
//	Description : Debug functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef DEBUG
#	define aiDebug						(1<<0)
#	define aiBrain						(1<<1)
#	define aiMotion						(1<<2)
#	define aiFrustum					(1<<3)
#	define aiFuncs						(1<<4)
#	define aiALife						(1<<5)
#	define aiGOAP						(1<<7)
#	define aiCover						(1<<8)
#	define aiAnimation					(1<<9)
#	define aiVision						(1<<10)
#	define aiMonsterDebug				(1<<11)
#	define aiStats						(1<<12)
#	define aiDestroy					(1<<13)
#	define aiSerialize					(1<<14)
#	define aiDialogs					(1<<15)
#	define aiInfoPortion				(1<<16)
#	define aiGOAPScript					(1<<17)
#	define aiGOAPObject					(1<<18)
#	define aiStalker					(1<<19)
#	define aiDrawGameGraph				(1<<20)
#	define aiDrawGameGraphStalkers		(1<<21)
#	define aiDrawGameGraphObjects		(1<<22)
#	define aiNilObjectAccess			(1<<23)
#	define aiDebugOnFrameAllocs			(1<<25)
#	define aiDrawVisibilityRays			(1<<26)
#	define aiAnimationStats				(1<<27)
#	define aiDrawGameGraphRealPos		(1<<28)
#endif // DEBUG

#ifndef MASTER_GOLD
#	define aiLua						(1<<6)
#	define aiIgnoreActor				(1<<24)
#	define aiObstaclesAvoiding			(1<<28)
#	define aiObstaclesAvoidingStatic	(1<<29)
#	define aiUseSmartCovers				(1<<30)
#	define aiUseSmartCoversAnimationSlot (1<<31)
	extern Flags32						psAI_Flags;
#endif // MASTER_GOLD