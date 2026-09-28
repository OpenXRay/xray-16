#include "stdafx.h"

#if defined(XR_PLATFORM_ANDROID)

#include "android_touch_controls.h"

#include <SDL.h>
#include <jni.h>

#include <atomic>

namespace
{
constexpr int TouchControlCount = 8;
std::atomic<std::uint32_t> g_touchControls{};

Uint32 touch_window_id()
{
    SDL_Window* window = SDL_GetMouseFocus();
    if (!window)
        window = SDL_GetKeyboardFocus();
    return window ? SDL_GetWindowID(window) : 0;
}
}

std::uint32_t AndroidTouchControlMask() noexcept
{
    return g_touchControls.load(std::memory_order_acquire);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openxray_app_XRayActivity_nativeSetTouchControl(
    JNIEnv*, jclass, jint control, jboolean pressed)
{
    if (control < 0 || control >= TouchControlCount)
        return;

    const std::uint32_t bit = std::uint32_t(1) << control;
    if (pressed)
        g_touchControls.fetch_or(bit, std::memory_order_release);
    else
        g_touchControls.fetch_and(~bit, std::memory_order_release);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openxray_app_XRayActivity_nativeMoveTouchMouse(
    JNIEnv*, jclass, jfloat deltaX, jfloat deltaY)
{
    SDL_Event event{};
    event.type = SDL_MOUSEMOTION;
    event.motion.type = SDL_MOUSEMOTION;
    event.motion.timestamp = SDL_GetTicks();
    event.motion.windowID = touch_window_id();
    event.motion.which = SDL_TOUCH_MOUSEID;
    event.motion.xrel = static_cast<Sint32>(deltaX);
    event.motion.yrel = static_cast<Sint32>(deltaY);

    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);
    event.motion.x = x + event.motion.xrel;
    event.motion.y = y + event.motion.yrel;
    SDL_PushEvent(&event);
}

extern "C" JNIEXPORT void JNICALL
Java_org_openxray_app_XRayActivity_nativeClickTouchMouse(
    JNIEnv*, jclass, jboolean pressed)
{
    SDL_Event event{};
    event.type = pressed ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    event.button.type = event.type;
    event.button.timestamp = SDL_GetTicks();
    event.button.windowID = touch_window_id();
    event.button.which = SDL_TOUCH_MOUSEID;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.button.clicks = 1;
    SDL_GetMouseState(&event.button.x, &event.button.y);
    SDL_PushEvent(&event);
}

#endif
