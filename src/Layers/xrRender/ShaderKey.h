#pragma once

#include "xrCore/xrstring.h"
#include <string>

namespace xray::render::fg {
    struct SPass;
    class dxRender_Visual;
}

namespace xray::render::fg {

struct ShaderKey {
    shared_str vsName;
    shared_str psName;
    shared_str gsName;
#ifdef USE_DX11
    shared_str hsName;
    shared_str dsName;
    shared_str csName;
#elif defined(USE_OGL)
    shared_str ppName;
#endif

    bool operator<(const ShaderKey& other) const {
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

#ifdef USE_DX11
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
#elif defined(USE_OGL)
        if (ppName.size()) {
            if (!result.empty()) result += "+";
            result += ppName.c_str();
        }
#endif

        return result.empty() ? "<no_shader>" : result;
    }

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

bool ExtractShaderKey(dxRender_Visual* visual, ShaderKey& outKey);

bool ExtractShaderKeyFromPass(SPass* pass, ShaderKey& outKey);

}
