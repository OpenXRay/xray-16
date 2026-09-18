#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

source "${SCRIPT_DIR}/common.sh"

CATEGORY="arms"
SKELETON_BASENAME="wpn_hand_01"
ANIMATION_BASENAMES=(
    "wpn_hand_ak74u_hud_animation"
)

parse_common_args "$@"

if [[ "${COMMON_SHOW_HELP}" -eq 1 ]]; then
    cat <<'USAGE'
Usage: run_arms_conversion.sh [options]

Options:
  --build-dir PATH        Directory containing xray_to_ozz_converter (default: auto-detected).
  --output-dir PATH       Destination for generated assets (default: tests/testdata/<category>).
  --texture-root PATH     Texture root passed to ozz_animation_viewer.
  --fsltx PATH            Override fsgame.ltx used for filesystem mounting.
  --viewer / --no-viewer  Force enable/disable viewer launch (default: auto if viewer exists).
  --help                  Show this help message.
USAGE
    exit 0
fi

if [[ ${#COMMON_ARGS_REMAINING[@]} -gt 0 ]]; then
    echo "Unexpected arguments: ${COMMON_ARGS_REMAINING[*]}" >&2
    exit 1
fi

BUILD_DIR_RESOLVED="$(resolve_build_dir "$ROOT_DIR")"
CONVERTER="$(converter_path "$BUILD_DIR_RESOLVED")"
VIEWER="$(viewer_path "$BUILD_DIR_RESOLVED")"

TESTDATA_ROOT="${ROOT_DIR}/src/xrAnimation/tests/testdata"
OUTPUT_DIR="${OUTPUT_ROOT:-${TESTDATA_ROOT}/${CATEGORY}}"
mkdir -p "${OUTPUT_DIR}"

SKELETON_PATH="${ROOT_DIR}/res/testdata/${CATEGORY}/${SKELETON_BASENAME}.ogf"
if [[ ! -f "${SKELETON_PATH}" ]]; then
    echo "Error: skeleton file not found: ${SKELETON_PATH}" >&2
    exit 1
fi

BUNDLE_OUTPUT="${OUTPUT_DIR}/${SKELETON_BASENAME}.ozzx"
run_converter "${CONVERTER}" bundle "${SKELETON_PATH}" "${BUNDLE_OUTPUT}"

# Still convert animations separately for viewer
FIRST_ANIMATION_OUTPUT=""
for animation_base in "${ANIMATION_BASENAMES[@]}"; do
    ANIMATION_PATH="${ROOT_DIR}/res/testdata/${CATEGORY}/${animation_base}.omf"
    ANIMATION_OUTPUT="${OUTPUT_DIR}/${animation_base}.ozz"
    if [[ -f "${ANIMATION_PATH}" ]]; then
        run_converter "${CONVERTER}" animation "${ANIMATION_PATH}" "${ANIMATION_OUTPUT}" "${SKELETON_PATH}" --optimize
        if [[ -z "${FIRST_ANIMATION_OUTPUT}" ]]; then
            FIRST_ANIMATION_OUTPUT="${ANIMATION_OUTPUT}"
        fi
    else
        echo "[ozz] warning: animation file not found: ${ANIMATION_PATH}" >&2
    fi
done

if [[ -z "${TEXTURE_ROOT}" ]]; then
    DEFAULT_TEXTURE_ROOT="/mnt/f/modding/Vanilla_Guns_noedits/unpacked_patches/basedata/textures"
    if [[ -d "${DEFAULT_TEXTURE_ROOT}" ]]; then
        TEXTURE_ROOT="${DEFAULT_TEXTURE_ROOT}"
    fi
fi
if [[ -z "${TEXTURE_ROOT}" ]]; then
    LOCAL_TEXTURE_ROOT="${ROOT_DIR}/res/testdata/textures"
    if [[ -d "${LOCAL_TEXTURE_ROOT}" ]]; then
        TEXTURE_ROOT="${LOCAL_TEXTURE_ROOT}"
    fi
fi

maybe_run_viewer "${VIEWER}" "${BUNDLE_OUTPUT}" "${FIRST_ANIMATION_OUTPUT}"
