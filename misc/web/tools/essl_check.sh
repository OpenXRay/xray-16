#!/bin/sh
set -e
cd "$(dirname "$0")/../../.."
ROOT=res/gamedata/shaders/gl
OUT=build/essl_check
mkdir -p "$OUT"
rm -f "$OUT"/*.log
clang++ -std=c++17 -O1 -o "$OUT/essl_check" misc/web/tools/essl_check.cpp

DEFINES="SMAP_size=2048 FP16_FILTER FP16_BLEND USE_HWSMAP USE_HWSMAP_PCF USE_BRANCHING USE_VTF USE_SOFT_WATER
SSR_QUALITY=3 SSR_HALF_DEPTH SSR_JITTER USE_SOFT_PARTICLES USE_DOF SUN_SHAFTS_QUALITY=2 SSAO_QUALITY=3 SUN_QUALITY=1
ALLOW_STEEPPARALLAX GBUFFER_OPTIMIZATION"

fail=0; total=0
for file in "$ROOT"/*.vs "$ROOT"/*.ps; do
    total=$((total + 1))
    case "$file" in *.vs) stage=vert;; *) stage=frag;; esac
    name=$(basename "$file")
    case "$name" in ssao_hdao_new.ps) continue;; esac # HLSL compute source, only used through DX11
    target="$OUT/$name.$stage"
    skin=SKIN_NONE
    if "$OUT/essl_check" "$ROOT" "$file" "$stage" "$target" $DEFINES > /dev/null 2>&1 && grep -q "v_model_skinned" "$target"; then
        skin=SKIN_1
    fi
    if ! "$OUT/essl_check" "$ROOT" "$file" "$stage" "$target" $DEFINES $skin > "$OUT/$name.log" 2>&1; then
        echo "EXPAND FAIL $name"; cat "$OUT/$name.log"; fail=$((fail + 1)); continue
    fi
    if ! grep -q "void main" "$target"; then
        continue # include-only file, never compiled on its own
    fi
    if ! glslangValidator -S "$stage" "$target" > "$OUT/$name.log" 2>&1; then
        echo "FAIL $name"; grep "ERROR" "$OUT/$name.log" | head -${ESSL_ERRORS:-3}; fail=$((fail + 1))
    fi
done
echo "$fail of $total shaders failed"
