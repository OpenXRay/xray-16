#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "core.h"

#include <cstring>
#include <memory>

namespace discord {

Result Core::Create(ClientId clientId, CreateFlags flags, Core** instance)
{
    if (!instance) {
        return Result::InternalError;
    }

    (*instance) = new Core();
    DiscordCreateParams params{};
    DiscordCreateParamsSetDefault(&params);
    params.client_id = clientId;
    params.flags = static_cast<EDiscordCreateFlags>(flags);
    params.events = nullptr;
    params.event_data = *instance;
    params.user_events = &UserManager::events_;
    params.activity_events = &ActivityManager::events_;
    params.relationship_events = &RelationshipManager::events_;
    params.overlay_events = &OverlayManager::events_;
    auto result = DiscordCreate(DISCORD_VERSION, &params, &((*instance)->internal_));
    if (result != DiscordResult_Ok || !(*instance)->internal_) {
        Destroy(instance);
    }

    return static_cast<Result>(result);
}

void Core::Destroy(Core** instance)
{
    delete (*instance);
    (*instance) = nullptr;
}

Core::~Core()
{
    if (internal_) {
        internal_->destroy(internal_);
        internal_ = nullptr;
    }
}

Result Core::RunCallbacks()
{
    auto result = internal_->run_callbacks(internal_);
    return static_cast<Result>(result);
}

void Core::SetLogHook(LogLevel minLevel, HookType* hook)
{
    static auto wrapper =
      [](void* callbackData, EDiscordLogLevel level, const char* message) -> void {
        if (!callbackData) {
            return;
        }
        const auto cb = (HookType*)callbackData;
        (cb)(static_cast<LogLevel>(level), message);
    };

    internal_->set_log_hook(internal_,
        static_cast<EDiscordLogLevel>(minLevel), static_cast<void*>(hook), wrapper);
}

discord::UserManager& Core::UserManager()
{
    if (!userManager_.internal_) {
        userManager_.internal_ = internal_->get_user_manager(internal_);
    }

    return userManager_;
}

discord::ActivityManager& Core::ActivityManager()
{
    if (!activityManager_.internal_) {
        activityManager_.internal_ = internal_->get_activity_manager(internal_);
    }

    return activityManager_;
}

discord::RelationshipManager& Core::RelationshipManager()
{
    if (!relationshipManager_.internal_) {
        relationshipManager_.internal_ = internal_->get_relationship_manager(internal_);
    }

    return relationshipManager_;
}

discord::OverlayManager& Core::OverlayManager()
{
    if (!overlayManager_.internal_) {
        overlayManager_.internal_ = internal_->get_overlay_manager(internal_);
    }

    return overlayManager_;
}

} // namespace discord
