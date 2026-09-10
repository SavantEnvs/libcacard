#!/usr/bin/env bash
#
# mayhem/test.sh — behavioral oracle for libcacard's 7816 APDU parser.
#
# Runs the CLEAN (non-sanitized), dynamically-linked probe built by
# mayhem/build.sh (linking the REAL src/card_7816.c) on fixed APDUs and asserts
# the EXACT decoded field values. These are known-answer tests: a PATCH that
# neuters the parser to a no-op (or the verify-repo sabotage shim that
# _exit(0)s the probe) produces empty output, so every assertion FAILS.
#
# Emits a CTRF summary + a compact `CTRF {...}` stdout marker; exits non-zero
# iff failed>0.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
SRC="${SRC:-/mayhem}"
cd "$SRC"

P=/mayhem/apdu_probe

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Fail loudly if build.sh did not produce the probe (a build bug, not a skip).
if [ ! -x "$P" ]; then
  echo "FATAL: $P missing/not executable — mayhem/build.sh did not build the oracle" >&2
  emit_ctrf "libcacard-apdu-kat" 0 1 0
  exit 1
fi

passed=0
failed=0

assert() {
  local label="$1" want="$2" got="$3"
  if [ "$got" = "$want" ]; then
    echo "PASS  $label -> '$got'"
    passed=$((passed + 1))
  else
    echo "FAIL  $label : want='$want' got='$got'"
    failed=$((failed + 1))
  fi
}

# --- 7816 APDU known-answer tests (src/card_7816.c: vcard_apdu_new) ---------
# SELECT-shaped Case-2S APDU: 00 A4 04 00 00  (len 5, L=1, Le byte 0 -> 256)
SELECT=00A4040000
assert "select status"  "0x9000" "$("$P" status  "$SELECT" 2>/dev/null)"
assert "select cla"     "0x00"   "$("$P" cla     "$SELECT" 2>/dev/null)"
assert "select ins"     "0xa4"   "$("$P" ins     "$SELECT" 2>/dev/null)"
assert "select p1"      "0x04"   "$("$P" p1      "$SELECT" 2>/dev/null)"
assert "select le"      "256"    "$("$P" le      "$SELECT" 2>/dev/null)"
assert "select type"    "0x00"   "$("$P" type    "$SELECT" 2>/dev/null)"
assert "select gentype" "0"      "$("$P" gentype "$SELECT" 2>/dev/null)"  # VCARD_7816_ISO

# Case-3S APDU with a 2-byte body: 00 A4 04 00 02 3F 00  (L=3, Lc=2, Le=0)
CASE3S=00A40400023F00
assert "case3s lc" "2" "$("$P" lc "$CASE3S" 2>/dev/null)"
assert "case3s le" "0" "$("$P" le "$CASE3S" 2>/dev/null)"

# Channel decode: cla low bits -> logical channel (03 A4 04 00 00 -> channel 3)
assert "channel 3" "3" "$("$P" channel 03A4040000 2>/dev/null)"

# Proprietary class 0xFF -> VCARD_7816_PTS (gen_type 2)
assert "pts gentype" "2" "$("$P" gentype FF00000000 2>/dev/null)"

# Too-short buffer (< 4 bytes) is rejected with WRONG_LENGTH (0x6700)
assert "short reject" "0x6700" "$("$P" status 00A4 2>/dev/null)"

emit_ctrf "libcacard-apdu-kat" "$passed" "$failed" 0
