#!/bin/sh
# Produce the evidence/ directory you submit with Part 2.
#
# Usage: ./evidence.sh DREXEL_ID      (or: make evidence ID=abc123)
#
# Runs:
#   1. the full test suite                       -> evidence/test-output.txt
#   2. merged traces for every test case          -> evidence/merged-<test>.txt
#   3. four personalized runs with your DROP and SIZES, window 8 and window 1
#                                                  -> evidence/*-p-*.txt
#   4. the head-of-line loss-rate experiment       -> evidence/hol-results.txt
#   5. a one-page summary                          -> evidence/SUMMARY.txt
#
# Files use the .txt extension so a repository-wide "*.log" ignore rule does
# not hide them. Takes about 4 minutes.

id="${1:-}"
if [ -z "$id" ]; then
    echo "usage: $0 DREXEL_ID   (for example: $0 abc123)" >&2
    exit 1
fi

make all >/dev/null || { echo "evidence: build failed" >&2; exit 1; }
mkdir -p evidence
./params.sh "$id" > evidence/params.txt || exit 1
. ./evidence/params.txt

echo "evidence: parameters: DROP=$DROP SIZES=$SIZES"

# 1-2. Test suite and merged traces.
echo "evidence: running test suite (about 45 s)"
./test.sh > evidence/test-output.txt 2>&1
for client_log in client-*.log; do
    [ -f "$client_log" ] || continue
    label=${client_log#client-}
    label=${label%.log}
    [ -f "server-$label.log" ] || continue
    ./dp-log "$client_log" "server-$label.log" > "evidence/merged-$label.txt" 2>/dev/null
done

# 3. Personalized runs.
run_personal() {
    label="$1"; mode="$2"; window="$3"
    echo "evidence: personalized run $label"
    ./dp-server -v > "evidence/server-$label.txt" 2>&1 &
    pid=$!
    sleep 0.2
    ./dp-client -v -m "$mode" -k "$SIZES" -D "$DROP" -w "$window" > "evidence/client-$label.txt" 2>&1
    wait "$pid"
    ./dp-log "evidence/client-$label.txt" "evidence/server-$label.txt" > "evidence/merged-$label.txt" 2>/dev/null
}
run_personal p-multi     multi  8
run_personal p-single    single 8
run_personal p-multi-w1  multi  1
run_personal p-single-w1 single 1

# 4. Loss-rate experiment.
echo "evidence: running HOL loss-rate experiment (about 2 minutes)"
./hol.sh 5 > evidence/hol-output.txt 2>&1
cp hol-results.txt evidence/hol-results.txt 2>/dev/null

# 5. Summary.
{
    echo "Part 2 evidence summary"
    echo "Generated: $(date)"
    echo
    cat evidence/params.txt
    echo
    echo "== Test suite =="
    grep '^test:.*\(PASS\|FAIL\)' evidence/test-output.txt
    echo
    for label in p-multi p-single p-multi-w1 p-single-w1; do
        echo "== $label =="
        grep -h '^dp-client: mode=' "evidence/client-$label.txt"
        grep -h '^dp-server: RESULT' "evidence/server-$label.txt"
        grep -h '^dp-server: verify:' "evidence/server-$label.txt"
        echo
    done
    echo "== HOL experiment =="
    cat evidence/hol-results.txt 2>/dev/null
} > evidence/SUMMARY.txt

echo "evidence: done. See evidence/SUMMARY.txt, then commit the evidence/ directory."
