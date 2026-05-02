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
    shared_str vsName;
    shared_str psName;
    shared_str gsName;
    shared_str hsName;
    shared_str dsName;
    shared_str csName;

    bool operator<(const ShaderKey& other) const {
        if (psName != other.psName) return psName < other.psName;
        if (vsName != other.vsName) return vsName < other.vsName;
        if (gsName != other.gsName) return gsName < other.gsName;
        if (hsName != other.hsName) return hsName < other.hsName;
        if (dsName != other.dsName) return dsName < other.dsName;
        if (csName != other.csName) return csName < other.csName;
        return false;
    }

    bool operator==(const ShaderKey& other) const {
        return vsName == other.vsName
            && psName == other.psName
            && gsName == other.gsName
            && hsName == other.hsName
            && dsName == other.dsName
            && csName == other.csName;
    }

    bool operator!=(const ShaderKey& other) const {
        return !(*this == other);
    }

    std::string ToString() const {
        std::string result;
        if (psName.size()) {
            result = psName.c_str();
        }
        if (vsName.size()) {
            if (!result.empty()) result += "+";
            result += vsName.c_str();
        }
        if (gsName.size()) {
            if (!result.empty()) result += "+";
            result += gsName.c_str();
        }
        if (hsName.size()) {
            if (!result.empty()) result += "+";
            result += hsName.c_str();
        }
        if (dsName.size()) {
            if (!result.empty()) result += "+";
            result += dsName.c_str();
        }
        if (csName.size()) {
            if (!result.empty()) result += "+";
            result += csName.c_str();
        }
        return result.empty() ? "<no_shader>" : result;
    }

    bool IsValid() const {
        return vsName.size() > 0 || psName.size() > 0 || gsName.size() > 0
            || hsName.size() > 0 || dsName.size() > 0 || csName.size() > 0;
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
