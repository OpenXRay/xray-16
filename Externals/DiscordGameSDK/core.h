#pragma once

#include "types.h"
#include "user_manager.h"
#include "activity_manager.h"
#include "relationship_manager.h"
#include "overlay_manager.h"

namespace discord {

class Core final {
public:
    using HookType = void(LogLevel, const char*);
public:
    static Result Create(ClientId clientId, CreateFlags flags, Core** instance);
    static void Destroy(Core** instance);

    ~Core();

    Result RunCallbacks();
    void SetLogHook(LogLevel minLevel, HookType* hook);

    discord::UserManager& UserManager();
    discord::ActivityManager& ActivityManager();
    discord::RelationshipManager& RelationshipManager();
    discord::OverlayManager& OverlayManager();

private:
    Core() = default;
    Core(Core const& rhs) = delete;
    Core& operator=(Core const& rhs) = delete;
    Core(Core&& rhs) = delete;
    Core& operator=(Core&& rhs) = delete;

    IDiscordCore* internal_;
    discord::UserManager userManager_;
    discord::ActivityManager activityManager_;
    discord::RelationshipManager relationshipManager_;
    discord::OverlayManager overlayManager_;
};

} // namespace discord
