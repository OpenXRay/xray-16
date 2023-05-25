#pragma once
#include "..\..\XrCore\xrCore.h"
#ifdef XREUI_EXPORTS
#define XREUI_API __declspec(dllexport)
#else
#define XREUI_API __declspec(dllimport)
#endif
#include "XrUI.h"
#include <d3d9.h>
#include "XrUIManager.h"

#define IMGUI_API XREUI_API
#define IMGUI_IMPL_API
using ImTextureID = IDirect3DBaseTexture9*;
#define ImTextureID ImTextureID 
#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define IMGUI_INCLUDE_IMGUI_USER_H

#include "imgui.h"