#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
out=${1:-$root/dist/bitspan.h}
mkdir -p "$(dirname "$out")"

{
    cat "$root/include/bitspan/bitspan.h"
    echo
    echo "#if defined(BITF_IMPLEMENTATION) && !defined(BITF_IMPLEMENTATION_DONE)"
    echo "#define BITF_IMPLEMENTATION_DONE"
    for src in "$root"/src/*.h "$root"/src/*.c; do
        [ -f "$src" ] && grep -v '^#include "' "$src"
    done
    echo "#endif"
} > "$out"
echo "$out"
