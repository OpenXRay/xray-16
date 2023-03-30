#pragma once

enum EEventType
{
    eventAnimationStart = u32(0),
    eventAnimationEnd,
    eventSoundStart,
    eventSoundEnd,
    eventParticlesStart,
    eventParticlesEnd,
    eventStep,
    eventTAChange,
    eventVelocityBounce,
};

class IEventData
{
};
