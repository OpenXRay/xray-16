file(SHA256 "${METAL_CONVERTER}" converter_hash)
file(SHA256 "${DXC_LIBRARY}" dxc_hash)
file(CONFIGURE OUTPUT "${OUTPUT}" CONTENT "#pragma once\n#define XR_METAL_SHADER_TOOLCHAIN_ID \"msc-@converter_hash@-dxc-@dxc_hash@\"\n" @ONLY)
