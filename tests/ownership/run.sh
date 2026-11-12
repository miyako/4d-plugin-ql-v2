#!/bin/bash
# Builds the synthetic-boundary ownership harness against a source tree with
# ASan/UBSan and runs every test in its own process.
#
#   tests/ownership/run.sh <source-root> <fixed|baseline>
#
# <source-root> provides 4DPlugin-QL.cpp and the bundled 4D Plugin API. The
# harness itself always comes from this script's directory, so the SAME
# harness can be run against an immutable baseline (git archive of the base
# SHA) and against the fixed tree.
#
# In "fixed" mode every test must pass. In "baseline" mode the tests named
# in EXPECTED_BASELINE_FAILURES must fail and every other test (the
# controls) must pass; anything else is a harness or model error.
set -euo pipefail

SRC=$(cd "$1" && pwd)
MODE=$2
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

EXPECTED_BASELINE_FAILURES="
defect_cmd_preview_metadata_double_dispose
defect_ob_set_s_double_dispose
defect_ob_set_s_empty_owner_leak
defect_ob_set_i_empty_picture_leak
defect_cmd_preview_attachments
defect_cmd_preview_no_attachments_collection_leak
defect_cmd_preview_null_object_leak
"

SAN="-g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined"
INC=(-I "$SRC/4D Plugin API" -I "$SRC" -I "$HERE")
WARN="-w"

# Like the Xcode target: 4DPluginAPI.c and 4DPlugin-QL.cpp are Objective-C, no ARC.
xcrun clang $SAN $WARN -x objective-c -fno-objc-arc "${INC[@]}" -Wno-multichar -Wno-error=int-conversion -c "$SRC/4D Plugin API/4DPluginAPI.c" -o "$OUT/api.o"
# harness.m #includes 4DPlugin-QL.cpp from $SRC.
xcrun clang $SAN $WARN -x objective-c -fno-objc-arc "${INC[@]}" -c "$HERE/harness.m" -o "$OUT/harness.o"
xcrun clang++ $SAN $WARN -std=c++17 "${INC[@]}" -c "$HERE/engine.cpp" -o "$OUT/engine.o"

xcrun clang++ $SAN "$OUT/api.o" "$OUT/harness.o" "$OUT/engine.o" \
    -framework Foundation -framework AppKit -framework QuickLook -framework Quartz \
    -framework ImageIO -framework CoreServices -o "$OUT/harness"

export ASAN_OPTIONS=abort_on_error=0:halt_on_error=1
export UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1

bad=0
for t in $("$OUT/harness" --list); do
    if "$OUT/harness" "$t" >"$OUT/$t.log" 2>&1; then got=pass; else got=fail; fi
    expect=pass
    if [ "$MODE" = baseline ] && grep -qx "$t" <<<"$EXPECTED_BASELINE_FAILURES"; then expect=fail; fi
    if [ "$got" = "$expect" ]; then status=OK; else status=UNEXPECTED; bad=1; fi
    printf '%-12s %-5s (expected %-4s) %s\n' "$status" "$got" "$expect" "$t"
    if [ "$got" = fail ]; then sed 's/^/    | /' "$OUT/$t.log"; fi
    # A sanitizer report or harness error is never an expected outcome.
    if grep -qE 'ERROR: AddressSanitizer|runtime error:|HARNESS ERROR' "$OUT/$t.log"; then
        echo "UNEXPECTED   sanitizer/harness error in $t"; bad=1
    fi
done

if [ $bad -ne 0 ]; then echo "RESULT ($MODE): FAILED"; exit 1; fi
echo "RESULT ($MODE): all outcomes as expected"
