#!/bin/sh
# Produce the evidence/ directory you submit with Part 1.
#
# Usage: ./evidence.sh DREXEL_ID      (or: make evidence ID=abc123)
#
# Runs:
#   1. the full test suite                      -> evidence/test-output.txt
#   2. merged traces for every test case         -> evidence/merged-<test>.txt
#   3. three personalized runs with your parameters:
#        p-data   the client drops DATA packet DATA_DROP once
#        p-ack    the server drops the ACK for packet ACK_DROP once
#        p-delay  0..DELAY ms random delay in both directions, no loss
#                                                -> evidence/*-p-*.txt
#   4. a one-page summary                        -> evidence/SUMMARY.txt
#
# Files use the .txt extension so a repository-wide "*.log" ignore rule does
# not hide them. Takes about 2 minutes.

id="${1:-}"
if [ -z "$id" ]; then
    echo "usage: $0 DREXEL_ID   (for example: $0 abc123)" >&2
    exit 1
fi

make all >/dev/null || { echo "evidence: build failed" >&2; exit 1; }
mkdir -p evidence
./params.sh "$id" > evidence/params.txt || exit 1
. ./evidence/params.txt
echo "evidence: parameters: DATA_DROP=$DATA_DROP ACK_DROP=$ACK_DROP SIZE_KB=$SIZE_KB DELAY=$DELAY"

echo "evidence: running test suite (about 90 s)"
./test.sh > evidence/test-output.txt 2>&1
for client_log in client-*.log; do
    [ -f "$client_log" ] || continue
    label=${client_log#client-}
    label=${label%.log}
    [ -f "server-$label.log" ] || continue
    ./dp-log "$client_log" "server-$label.log" > "evidence/merged-$label.txt" 2>/dev/null
done

run_personal() {
    label="$1"; server_args="$2"; client_args="$3"
    echo "evidence: personalized run $label"
    ./dp-server -v $server_args > "evidence/server-$label.txt" 2>&1 &
    pid=$!
    sleep 0.2
    ./dp-client -v -k "$SIZE_KB" $client_args > "evidence/client-$label.txt" 2>&1
    wait "$pid"
    ./dp-log "evidence/client-$label.txt" "evidence/server-$label.txt" > "evidence/merged-$label.txt" 2>/dev/null
}
run_personal p-data  ""                       "-D $DATA_DROP"
run_personal p-ack   "-D $ACK_DROP"           ""
run_personal p-delay "-d $DELAY -s 11"        "-d $DELAY -s 12"

{
    echo "Part 1 evidence summary"
    echo "Generated: $(date)"
    echo
    cat evidence/params.txt
    echo
    echo "== Test suite =="
    grep '^test:.*\(PASS\|FAIL\)' evidence/test-output.txt
    for label in p-data p-ack p-delay; do
        echo
        echo "== $label =="
        grep -h '^dp-client: \(sent\|close\|network\)' "evidence/client-$label.txt"
        grep -h '^dp-server: \(received\|verify\|close\)' "evidence/server-$label.txt"
        echo "TIMEOUT lines: $(grep -c TIMEOUT "evidence/client-$label.txt")  DUP lines: $(grep -c ' DUP ' "evidence/server-$label.txt")"
    done
} > evidence/SUMMARY.txt

echo "evidence: done. See evidence/SUMMARY.txt, then commit the evidence/ directory."
