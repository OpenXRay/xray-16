#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <arch> <configuration>"
    exit 1
fi

ARCH="$1"
CONFIGURATION="$2"
SKIP_DMG="${OPENXRAY_SKIP_DMG:-0}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BIN_DIR="${ROOT_DIR}/bin/${ARCH}/${CONFIGURATION}"
ARTIFACTS_DIR="${ROOT_DIR}/build/artifacts"

APP_DIR="${ARTIFACTS_DIR}/OpenXRay.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
LIBS_DIR="${CONTENTS_DIR}/libs"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"
OXR_RES_DIR="${RESOURCES_DIR}/openxray"

if [[ ! -x "${BIN_DIR}/xr_3da" ]]; then
    echo "Cannot find executable: ${BIN_DIR}/xr_3da"
    exit 1
fi

required_tools=(install_name_tool dylibbundler ditto)
if [[ "${SKIP_DMG}" != "1" ]]; then
    required_tools+=(hdiutil)
fi

for tool in "${required_tools[@]}"; do
    if ! command -v "${tool}" >/dev/null 2>&1; then
        echo "Required tool is missing: ${tool}"
        exit 1
    fi
done

mkdir -p "${ARTIFACTS_DIR}"
rm -rf "${APP_DIR}"
mkdir -p "${MACOS_DIR}" "${LIBS_DIR}" "${OXR_RES_DIR}"

cat > "${CONTENTS_DIR}/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>OpenXRay</string>
    <key>CFBundleDisplayName</key>
    <string>OpenXRay</string>
    <key>CFBundleIdentifier</key>
    <string>org.openxray.xray-16</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleExecutable</key>
    <string>xr_3da</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>LSApplicationCategoryType</key>
    <string>public.app-category.games</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
PLIST

printf 'APPL????' > "${CONTENTS_DIR}/PkgInfo"

cp "${BIN_DIR}/xr_3da" "${MACOS_DIR}/xr_3da"
chmod +x "${MACOS_DIR}/xr_3da"

find "${BIN_DIR}" -maxdepth 1 -type f -name '*.dylib' -exec cp {} "${LIBS_DIR}/" \;

# Bundle only open-source engine resources from this repository.
cp "${ROOT_DIR}/res/fsgame.ltx" "${OXR_RES_DIR}/fsgame.ltx"
git -C "${ROOT_DIR}/res" ls-files -z gamedata | rsync -a --from0 --files-from=- "${ROOT_DIR}/res/" "${OXR_RES_DIR}/"

# Bundle non-system dynamic libraries (Homebrew deps etc.).
dylibbundler \
    -of -cd -b \
    -x "${MACOS_DIR}/xr_3da" \
    -d "${LIBS_DIR}" \
    -s "${BIN_DIR}" \
    -s "${LIBS_DIR}"

reset_rpaths() {
    local binary="$1"

    while install_name_tool -delete_rpath "@executable_path/../libs" "${binary}" >/dev/null 2>&1; do
        :
    done
    while install_name_tool -delete_rpath "@executable_path/../libs/" "${binary}" >/dev/null 2>&1; do
        :
    done

    install_name_tool -add_rpath "@executable_path/../libs" "${binary}"
    codesign --force --deep --preserve-metadata=entitlements,requirements,flags,runtime --sign - "${binary}" >/dev/null
}

# dylibbundler may leave duplicate LC_RPATH commands, which dyld rejects on newer macOS.
reset_rpaths "${MACOS_DIR}/xr_3da"
for lib in "${LIBS_DIR}"/*.dylib; do
    [[ -e "${lib}" ]] || continue
    reset_rpaths "${lib}"
done

codesign --force --deep --sign - "${APP_DIR}" >/dev/null

APP_ZIP="${ARTIFACTS_DIR}/openxray-${CONFIGURATION}-${ARCH}.app.zip"
DMG_PATH="${ARTIFACTS_DIR}/openxray-${CONFIGURATION}-${ARCH}.dmg"
DMG_ROOT="${ARTIFACTS_DIR}/dmg-root"

rm -f "${APP_ZIP}" "${DMG_PATH}"
ditto -c -k --sequesterRsrc --keepParent "${APP_DIR}" "${APP_ZIP}"

echo "Created:"
echo "  ${APP_ZIP}"
if [[ "${SKIP_DMG}" == "1" ]]; then
    echo "Skipped DMG creation (OPENXRAY_SKIP_DMG=1)"
else
    rm -rf "${DMG_ROOT}"
    mkdir -p "${DMG_ROOT}"
    ditto "${APP_DIR}" "${DMG_ROOT}/OpenXRay.app"
    ln -s /Applications "${DMG_ROOT}/Applications"
    hdiutil create -volname "OpenXRay ${CONFIGURATION} ${ARCH}" -srcfolder "${DMG_ROOT}" -format UDZO -ov "${DMG_PATH}"
    rm -rf "${DMG_ROOT}"
    echo "  ${DMG_PATH}"
fi
