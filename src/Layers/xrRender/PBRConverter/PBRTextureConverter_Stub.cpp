#include "stdafx.h"
#include "PBRTextureConverter.h"

namespace xray::render::pbr {

TextureInventory BuildTextureInventory(const TextureScanConfig&) {
    return {};
}

bool VerifyPBROutputs(const TextureInventory&, const PBRConversionParams&) {
    return false;
}

bool ConvertTexturesToPBR(const TextureInventory&, const PBRConversionParams&,
                          PBRConversionStats&, ProgressCallback) {
    return false;
}

bool ConvertSingleTextureToPBR(const char*, const char*, const PBRConversionParams&) {
    return false;
}

bool ConsolidatePBRTextures(const xr_string&, ConsolidationStats&, ProgressCallback) {
    return false;
}

}
