#pragma once

#include "../RenderContext/ResourceHandle.h"

// Modern ResourceManager Handle Extensions
// Week 1 - Day 1: Task 1.1
//
// Note: The core generational handle system is already implemented in
// RenderContext/ResourceHandle.h. This file simply imports those types
// and provides any additional convenience types needed for the ResourceManager.

namespace xray::render::resources {

// ═══════════════════════════════════════════════════
//  IMPORT EXISTING HANDLE TYPES
// ═══════════════════════════════════════════════════

// Import base handle types from RenderContext
using xray::render::fg::ResourceHandle;
using xray::render::fg::TextureHandle;
using xray::render::fg::BufferHandle;
using xray::render::fg::SamplerHandle;

// Note: We reuse the existing handle types from RenderContext rather than
// creating separate streaming-specific handles. The streaming metadata
// (refCount, priority, lastAccessTime, etc.) is stored in the manager's
// metadata structures (TextureMetadata, BufferMetadata), not in the handles.
//
// This keeps handles lightweight (just index + generation) and allows them
// to be passed around efficiently.

// ═══════════════════════════════════════════════════
//  CONSTANTS
// ═══════════════════════════════════════════════════

constexpr u32 INVALID_RESOURCE_INDEX = 0xFFFFFFFF;

} // namespace xray::render::resources
