////////////////////////////////////////////////////////////////////////////
//	Module 		: object_handler_space.h
//	Created 	: 08.05.2004
//  Modified 	: 08.05.2004
//	Author		: Dmitriy Iassenev
//	Description : Object handler space
////////////////////////////////////////////////////////////////////////////

#pragma once

namespace ObjectHandlerSpace
{
enum EWorldProperties : u32
{
    eWorldPropertyItemID = u32(0),
    eWorldPropertyHidden,
    eWorldPropertyShown,
    eWorldPropertyStrapped,
    eWorldPropertyStrapped2Idle,
    eWorldPropertySwitch1,
    eWorldPropertySwitch2,
    eWorldPropertyAimed1,
    eWorldPropertyAimed2,
    eWorldPropertyAiming1,
    eWorldPropertyAiming2,
    eWorldPropertyEmpty1,
    eWorldPropertyEmpty2,
    eWorldPropertyFull1,
    eWorldPropertyFull2,
    eWorldPropertyReady1,
    eWorldPropertyReady2,
    eWorldPropertyFiring1,
    eWorldPropertyFiringNoReload1,
    eWorldPropertyFiring2,
    eWorldPropertyAmmo1,
    eWorldPropertyAmmo2,
    eWorldPropertyIdle,
    eWorldPropertyIdleStrap,
    eWorldPropertyDropped,
    eWorldPropertyQueueWait1,
    eWorldPropertyQueueWait2,
    eWorldPropertyAimingReady1,
    eWorldPropertyAimingReady2,
    eWorldPropertyAimForceFull1,
    eWorldPropertyAimForceFull2,

    eWorldPropertyThrowStarted,
    eWorldPropertyThrowIdle,
    eWorldPropertyThrow,
    eWorldPropertyThreaten,

    eWorldPropertyPrepared,
    eWorldPropertyUsed,
    eWorldPropertyUseEnough,

    eWorldPropertyNoItems = u32((u16(-1) << 16) | eWorldPropertyItemID),
    eWorldPropertyNoItemsIdle = u32((u16(-1) << 16) | eWorldPropertyIdle),
    eWorldPropertyDummy = u32(-1),
};

enum EWorldOperators
{
    eWorldOperatorShow = u32(0),
    eWorldOperatorDoShow,
    eWorldOperatorHide,
    eWorldOperatorDrop,
    eWorldOperatorStrapping,
    eWorldOperatorStrapping2Idle,
    eWorldOperatorUnstrapping,
    eWorldOperatorUnstrapping2Idle,
    eWorldOperatorStrapped,
    eWorldOperatorIdle,
    eWorldOperatorAim1,
    eWorldOperatorAim2,
    eWorldOperatorAimForceFull1,
    eWorldOperatorAimForceFull2,
    eWorldOperatorReload1,
    eWorldOperatorReload2,
    eWorldOperatorForceReload1,
    eWorldOperatorForceReload2,
    eWorldOperatorFire1,
    eWorldOperatorFireNoReload,
    eWorldOperatorFire2,
    eWorldOperatorSwitch1,
    eWorldOperatorSwitch2,
    eWorldOperatorQueueWait1,
    eWorldOperatorQueueWait2,
    eWorldOperatorAimingReady1,
    eWorldOperatorAimingReady2,
    eWorldOperatorGetAmmo1,
    eWorldOperatorGetAmmo2,

    eWorldOperatorThrowStart,
    eWorldOperatorThrowIdle,
    eWorldOperatorThrow,
    eWorldOperatorThreaten,
    eWorldOperatorAfterThreaten,

    eWorldOperatorPrepare,
    eWorldOperatorUse,

    eWorldOperatorNoItemsIdle = u32((u16(-1) << 16) | eWorldOperatorIdle),
    eWorldOperatorDummy = u32(-1),
};
};
