#pragma once

namespace GameObject
{
    enum ECallbackType : u32
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

        // Alundaio: added defines
        eActorHudAnimationEnd,

        // AVO: custom callbacks
        // Input
        eKeyPress,
        eKeyRelease,
        eKeyHold,
        eMouseMove,
        eMouseWheel,

        // Inventory
        eItemToBelt,
        eItemToSlot,
        eItemToRuck,

        // Weapon
        eOnWeaponZoomIn,
        eOnWeaponZoomOut,
        eOnWeaponJammed,
        eOnWeaponFired,
        eOnWeaponMagazineEmpty,
        eActorBeforeDeath,

        // Vehicle
        eAttachVehicle,
        eDetachVehicle,
        eUseVehicle,
        eOnFootStep,
        // -AVO

        eOnWeaponLowered,
        eOnWeaponRaised,

        eDummy = u32(-1),
    };
};
