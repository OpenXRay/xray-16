#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <arch> <configuration>"
    exit 1
fi

ARCH="$1"
CONFIGURATION="$2"

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

for tool in install_name_tool dylibbundler ditto hdiutil; do
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
cp -R "${ROOT_DIR}/res/gamedata" "${OXR_RES_DIR}/gamedata"

# Ensure runtime can resolve in-bundle libraries.
install_name_tool -add_rpath "@executable_path/../libs" "${MACOS_DIR}/xr_3da" || true
for lib in "${LIBS_DIR}"/*.dylib; do
    [[ -e "${lib}" ]] || continue
    install_name_tool -add_rpath "@loader_path" "${lib}" || true
done

# Bundle non-system dynamic libraries (Homebrew deps etc.).
dylibbundler -of -cd -b -x "${MACOS_DIR}/xr_3da" -d "${LIBS_DIR}"

APP_ZIP="${ARTIFACTS_DIR}/openxray-${CONFIGURATION}-${ARCH}.app.zip"
DMG_PATH="${ARTIFACTS_DIR}/openxray-${CONFIGURATION}-${ARCH}.dmg"

rm -f "${APP_ZIP}" "${DMG_PATH}"
ditto -c -k --sequesterRsrc --keepParent "${APP_DIR}" "${APP_ZIP}"
hdiutil create -volname "OpenXRay ${CONFIGURATION} ${ARCH}" -srcfolder "${APP_DIR}" -format UDZO -ov "${DMG_PATH}"

echo "Created:"
echo "  ${APP_ZIP}"
echo "  ${DMG_PATH}"
