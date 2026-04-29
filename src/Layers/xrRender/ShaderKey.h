// xrRender/ShaderKey.h
#pragma once

#include "xrCore/xrstring.h"
#include <string>

// Forward declarations
namespace xray::render::fg {
    struct SPass;
    class dxRender_Visual;
}

namespace xray::render::fg {

// ═══════════════════════════════════════════════════
//  SHADER KEY
// ═══════════════════════════════════════════════════
//
// Production-safe composite key for shader identification.
// Uses shader resource names (cName) instead of debug-only visual names.
//
// Benefits:
// - Available in all builds (not DEBUG-only)
// - More accurate (keys on actual shader combination)
// - Efficient (shared_str pointer comparison)
//
struct ShaderKey {
    shared_str vsName;  // Vertex shader
    shared_str psName;  // Pixel shader
    shared_str gsName;  // Geometry shader (optional)
#ifdef USE_DX11
    shared_str hsName;  // Hull shader (optional, DX11+)
    shared_str dsName;  // Domain shader (optional, DX11+)
    shared_str csName;  // Compute shader (optional, DX11+)
#elif defined(USE_OGL)
    shared_str ppName;  // Program pipeline (optional, OpenGL)
#endif

    // Comparison operator for map/set usage
    bool operator<(const ShaderKey& other) const {
        // Compare in order of importance
        if (psName != other.psName) return psName < other.psName;
        if (vsName != other.vsName) return vsName < other.vsName;
        if (gsName != other.gsName) return gsName < other.gsName;
#ifdef USE_DX11
        if (hsName != other.hsName) return hsName < other.hsName;
        if (dsName != other.dsName) return dsName < other.dsName;
        if (csName != other.csName) return csName < other.csName;
#elif defined(USE_OGL)
        if (ppName != other.ppName) return ppName < other.ppName;
#endif
        return false;
    }

    // Equality operator
    bool operator==(const ShaderKey& other) const {
        return vsName == other.vsName &&
               psName == other.psName &&
               gsName == other.gsName
#ifdef USE_DX11
               && hsName == other.hsName
               && dsName == other.dsName
               && csName == other.csName
#elif defined(USE_OGL)
               && ppName == other.ppName
#endif
               ;
    }

    bool operator!=(const ShaderKey& other) const {
        return !(*this == other);
    }

    // Generate human-readable debug string
    // Primary use: logging and debugging
    std::string ToString() const {
        std::string result;

        // Start with pixel shader (most important)
        if (psName.size()) {
            result = psName.c_str();
        }

        // Add vertex shader
        if (vsName.size()) {
            if (!result.empty()) result += "+";
            result += vsName.c_str();
        }

        // Add geometry shader
        if (gsName.size()) {
            if (!result.empty()) result += "+";
            result += gsName.c_str();
        }

#ifdef USE_DX11
        // Add hull shader
        if (hsName.size()) {
            if (!result.empty()) result += "+";
            result += hsName.c_str();
        }

        // Add domain shader
        if (dsName.size()) {
            if (!result.empty()) result += "+";
            result += dsName.c_str();
        }

        // Add compute shader
        if (csName.size()) {
            if (!result.empty()) result += "+";
            result += csName.c_str();
        }
#elif defined(USE_OGL)
        // Add program pipeline
        if (ppName.size()) {
            if (!result.empty()) result += "+";
            result += ppName.c_str();
        }
#endif

        return result.empty() ? "<no_shader>" : result;
    }

    // Check if key is valid (has at least one shader)
    bool IsValid() const {
        return vsName.size() > 0 || psName.size() > 0 || gsName.size() > 0
#ifdef USE_DX11
               || hsName.size() > 0 || dsName.size() > 0 || csName.size() > 0
#elif defined(USE_OGL)
               || ppName.size() > 0
#endif
               ;
    }
};

// ═══════════════════════════════════════════════════
//  SHADER KEY EXTRACTION
// ═══════════════════════════════════════════════════

// Extract shader key from a visual's shader pass
// Returns true if successful, false if visual/shader is invalid
bool ExtractShaderKey(dxRender_Visual* visual, ShaderKey& outKey);

// Extract shader key from a pass directly
// Returns true if successful, false if pass is invalid
bool ExtractShaderKeyFromPass(SPass* pass, ShaderKey& outKey);

} // namespace xray::render::fg
