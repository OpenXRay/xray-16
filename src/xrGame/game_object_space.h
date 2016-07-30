#pragma once

namespace GameObject
{
    enum ECallbackType
    {
		eTradeStart = u32(0),
		eTradeStop,
		eTradeSellBuyItem,
		eTradePerformTradeOperation,

		eZoneEnter,
		eZoneExit,
		eExitLevelBorder,
		eEnterLevelBorder,
		eDeath,

		ePatrolPathInPoint,

		eInventoryPda,
		eInventoryInfo,
		eArticleInfo,
		eTaskStateChange,
		eMapLocationAdded,

		eUseObject,

		eHit,

		eSound,

		eActionTypeMovement,
		eActionTypeWatch,
		eActionTypeRemoved,
		eActionTypeAnimation,
		eActionTypeSound,
		eActionTypeParticle,
		eActionTypeObject,

		eActorSleep,

		eHelicopterOnPoint,
		eHelicopterOnHit,

		eOnItemTake,
		eOnItemDrop,

		eScriptAnimation,

		eTraderGlobalAnimationRequest,
		eTraderHeadAnimationRequest,
		eTraderSoundEnd,

		eInvBoxItemTake,
		eWeaponNoAmmoAvailable,
		//Alundaio: added defines
		eActorHudAnimationEnd,
        //AVO: custom callbacks
#ifdef INPUT_CALLBACKS
		// input
        eKeyPress,
        eKeyRelease,
        eKeyHold,
#endif
#ifdef MOUSE_INPUT_CALLBACKS
        eMouseMove,
        eMouseWheel,
#endif
#ifdef EXTENDED_ITEM_CALLBACKS
        // inventory
        eItemToBelt,
        eItemToSlot,
        eItemToRuck,
#endif
#ifdef	EXTENDED_WEAPON_CALLBACKS
		// weapon
		eOnWeaponZoomIn,
		eOnWeaponZoomOut,
		eOnWeaponJammed,
		eOnWeaponMagazineEmpty,
#endif
        // actor
#ifdef ACTOR_BEFORE_DEATH_CALLBACK
        eActorBeforeDeath,
#endif
#ifdef ENABLE_CAR
		// vehicle
		eAttachVehicle,
		eDetachVehicle,
		eUseVehicle,
#endif
        //-AVO

        eDummy = u32(-1),
    };
};
