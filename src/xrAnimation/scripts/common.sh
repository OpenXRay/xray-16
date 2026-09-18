#!/usr/bin/env bash

# Guard against multiple inclusion.
if [[ -n "${XRANIMATION_SCRIPTS_COMMON_SH:-}" ]]; then
    return 0
fi
XRANIMATION_SCRIPTS_COMMON_SH=1

EXE_SUFFIX=""
case "$(uname -s 2>/dev/null)" in
    MINGW*|MSYS*|CYGWIN*|Windows_NT*)
        EXE_SUFFIX=".exe"
        ;;
esac

BUILD_DIR="${BUILD_DIR:-}"
OUTPUT_ROOT="${OUTPUT_ROOT:-}"
TEXTURE_ROOT="${TEXTURE_ROOT:-${XR_TEXTURE_ROOT:-}}"
RUN_VIEWER=${RUN_VIEWER:-1}
COMMON_SHOW_HELP=0
COMMON_ARGS_REMAINING=()
FSLTX_PATH="${FSLTX_PATH:-${XR_FSLTX_PATH:-}}"
COMMON_CONVERTER_PREFIX=()

update_converter_prefix() {
    COMMON_CONVERTER_PREFIX=()
    local resolved_fsltx="$FSLTX_PATH"

    if [[ -z "$resolved_fsltx" ]]; then
        local -a default_candidates=()
        if [[ -n "${XR_FSLTX_DEFAULTS:-}" ]]; then
            local saved_ifs="$IFS"
            IFS=':'
            read -r -a default_candidates <<< "${XR_FSLTX_DEFAULTS}"
            IFS="$saved_ifs"
        fi
        default_candidates+=("/mnt/c/games/scop/fsgame.ltx")

        for candidate in "${default_candidates[@]}"; do
            if [[ -n "$candidate" && -f "$candidate" ]]; then
                resolved_fsltx="$candidate"
                break
            fi
        done
        FSLTX_PATH="$resolved_fsltx"
    fi

    if [[ -n "$resolved_fsltx" ]]; then
        if [[ -f "$resolved_fsltx" ]]; then
            COMMON_CONVERTER_PREFIX=(-fsltx "$resolved_fsltx")
        else
            echo "[ozz] warning: specified --fsltx path '$resolved_fsltx' not found; continuing without filesystem override" >&2
        fi
    fi
}

parse_common_args() {
    COMMON_ARGS_REMAINING=()
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --build-dir)
                if [[ $# -lt 2 ]]; then
                    echo "Error: --build-dir requires a path" >&2
                    exit 1
                fi
                BUILD_DIR="$2"
                shift 2
                ;;
            --output-dir|--output-root)
                if [[ $# -lt 2 ]]; then
                    echo "Error: $1 requires a path" >&2
                    exit 1
                fi
                OUTPUT_ROOT="$2"
                shift 2
                ;;
            --texture-root)
                if [[ $# -lt 2 ]]; then
                    echo "Error: --texture-root requires a path" >&2
                    exit 1
                fi
                TEXTURE_ROOT="$2"
                shift 2
                ;;
            --fsltx)
                if [[ $# -lt 2 ]]; then
                    echo "Error: --fsltx requires a path" >&2
                    exit 1
                fi
                FSLTX_PATH="$2"
                shift 2
                ;;
            --viewer)
                RUN_VIEWER=1
                shift
                ;;
            --no-viewer)
                RUN_VIEWER=0
                shift
                ;;
            --help|-h)
                COMMON_SHOW_HELP=1
                shift
                ;;
            --)
                shift
                while [[ $# -gt 0 ]]; do
                    COMMON_ARGS_REMAINING+=("$1")
                    shift
                done
                break
                ;;
            *)
                COMMON_ARGS_REMAINING+=("$1")
                shift
                ;;
        esac
    done
    update_converter_prefix
}

resolve_build_dir() {
    local project_root="$1"

    if [[ -n "$BUILD_DIR" ]]; then
        if [[ -d "$BUILD_DIR" && -x "$BUILD_DIR/xray_to_ozz_converter${EXE_SUFFIX}" ]]; then
            echo "$BUILD_DIR"
            return
        fi
        echo "Error: --build-dir path '$BUILD_DIR' does not contain xray_to_ozz_converter${EXE_SUFFIX}" >&2
        exit 1
    fi

    local env_candidates=(
        "${OZZ_UTILS_BUILD_DIR:-}"
        "${XR_OZZ_BUILD_DIR:-}"
        "${XR_OZZ_BIN_DIR:-}"
    )
    local candidate
    for candidate in "${env_candidates[@]}"; do
        if [[ -n "$candidate" && -d "$candidate" && -x "$candidate/xray_to_ozz_converter${EXE_SUFFIX}" ]]; then
            echo "$candidate"
            return
        fi
    done

    local -a lookup_subdirs=(
        "ozz_utils/bin/Debug"
        "ozz_utils/bin/Release"
        "ozz_utils/bin/RelWithDebInfo"
        "ozz_utils/bin/Mixed"
        "ozz_utils/bin/x64/Debug"
        "ozz_utils/bin/x64/Release"
        "ozz_utils/bin/x64/RelWithDebInfo"
        "ozz_utils/bin/x64/Mixed"
        "ozz_utils/bin/x86_64/Debug"
        "ozz_utils/bin/x86_64/Release"
        "ozz_utils/bin/x86_64/RelWithDebInfo"
        "ozz_utils/bin/x86_64/Mixed"
        "bin/x64/Debug"
        "bin/x64/Release"
        "bin/x64/RelWithDebInfo"
        "bin/x64/Mixed"
        "bin/x86_64/Debug"
        "bin/x86_64/Release"
        "bin/x86_64/RelWithDebInfo"
        "bin/x86_64/Mixed"
        "build/bin/Debug"
        "build/bin/Release"
        "build/bin/RelWithDebInfo"
        "build/bin/Mixed"
    )

    for subdir in "${lookup_subdirs[@]}"; do
        candidate="${project_root}/${subdir}"
        if [[ -d "$candidate" && -x "$candidate/xray_to_ozz_converter${EXE_SUFFIX}" ]]; then
            echo "$candidate"
            return
        fi
    done

    echo "Error: unable to locate xray_to_ozz_converter${EXE_SUFFIX}. Use --build-dir or set OZZ_UTILS_BUILD_DIR." >&2
    exit 1
}

converter_path() {
    local base="$1"
    local path="${base}/xray_to_ozz_converter${EXE_SUFFIX}"
    if [[ ! -x "$path" ]]; then
        echo "Error: converter not found at $path" >&2
        exit 1
    fi
    echo "$path"
}

viewer_path() {
    local base="$1"
    local path="${base}/ozz_animation_viewer${EXE_SUFFIX}"
    if [[ -x "$path" ]]; then
        echo "$path"
    else
        echo ""
    fi
}

run_converter() {
    local converter="$1"
    shift
    local args=()
    if [[ ${#COMMON_CONVERTER_PREFIX[@]} -gt 0 ]]; then
        args+=("${COMMON_CONVERTER_PREFIX[@]}")
    fi
    args+=("$@")
    echo "[ozz] running: $(basename "$converter") ${args[*]}" >&2
    "$converter" "${args[@]}"
}

maybe_run_viewer() {
    local viewer="$1"
    local bundle="$2"
    local animation="${3:-}"

    if [[ "${RUN_VIEWER:-1}" -eq 0 ]]; then
        echo "[ozz] viewer launch skipped (--no-viewer)" >&2
        return
    fi

    if [[ -z "$viewer" || ! -x "$viewer" ]]; then
        echo "[ozz] ozz_animation_viewer not found; skipping preview" >&2
        return
    fi

    local args=("--bundle=${bundle}")
    if [[ -n "${TEXTURE_ROOT:-}" ]]; then
        args+=("--texture_root=${TEXTURE_ROOT}")
    fi
    if [[ -n "$animation" && -f "$animation" ]]; then
        args+=("--animation=${animation}")
    fi

    echo "[ozz] launching viewer: $(basename "$viewer") ${args[*]}" >&2
    "$viewer" "${args[@]}"
}
