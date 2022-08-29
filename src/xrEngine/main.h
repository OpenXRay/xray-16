#pragma once

ENGINE_API void InitEngine();
ENGINE_API void destroyEngine();

ENGINE_API void InitSettings();
ENGINE_API void destroySettings();

ENGINE_API void InitConsole();
ENGINE_API void destroyConsole();

ENGINE_API void InitInput();
ENGINE_API void destroyInput();

ENGINE_API void InitSoundDeviceList();
ENGINE_API void InitSound();
ENGINE_API void destroySound();

ENGINE_API void Startup();
ENGINE_API int RunApplication();
