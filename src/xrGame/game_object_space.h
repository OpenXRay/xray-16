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

    //Alundaio: added defines
    eActorHudAnimationEnd,

    //AVO: custom callbacks
    // Input
    eKeyPress,
    eKeyRelease,
    eKeyHold,
    eMouseMove,
    eMouseWheel,
    eControllerPress,
    eControllerRelease,
    eControllerHold,
    eControllerAttitudeChange,
    // Inventory
    eItemToBelt,
    eItemToSlot,
    eItemToRuck,
    // weapon
    eOnWeaponZoomIn,
    eOnWeaponZoomOut,
    eOnWeaponJammed,
    eOnWeaponMagazineEmpty,
    // Actor
    eActorBeforeDeath,
    // vehicle
    eAttachVehicle,
    eDetachVehicle,
    eUseVehicle,
    //-AVO

    eDummy = u32(-1),
};
};
