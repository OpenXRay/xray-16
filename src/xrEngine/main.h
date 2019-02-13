#pragma once

ENGINE_API void InitEngine();
ENGINE_API void DestroyEngine();

ENGINE_API void InitSettings();
ENGINE_API void DestroySettings();

ENGINE_API void InitConsole();
ENGINE_API void DestroyConsole();

ENGINE_API void InitInput();
ENGINE_API void DestroyInput();

ENGINE_API void InitSound();
ENGINE_API void DestroySound();

ENGINE_API void Startup();
ENGINE_API int RunApplication();
