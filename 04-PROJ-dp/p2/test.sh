#!/bin/sh

# Part 2 regression tests. Runs every case, records each result, and returns
# non-zero only after all cases have been attempted.
#
# A case passes only when BOTH endpoints succeed:
#   - the client exits 0 (connect, send, and close all completed), and
#   - the server exits 0, reports exactly the expected byte count, and
#     verifies every file (every byte delivered once, in the right place).
# The two HOL cases also check WHEN file 3 finished (see below).

failures=0
SERVER_WAIT_SECS=15
DEFAULT_BYTES=172032      # default files: 128 + 32 + 8 KB

run_case() {
    label="$1"
    bytes="$2"
    server_args="$3"
    client_args="$4"
    hol_check="$5"        # "", "fast", or "slow"

    printf "test: %s\n" "$label"

    ./dp-server $server_args -v >"server-${label}.log" 2>&1 &
    server_pid=$!
    sleep 0.2

    ./dp-client -v $client_args >"client-${label}.log" 2>&1
    client_rc=$?

    limit=$((SERVER_WAIT_SECS * 5))
    [ "$client_rc" -eq 0 ] || limit=5
    waited=0
    while kill -0 "$server_pid" 2>/dev/null && [ "$waited" -lt "$limit" ]; do
        sleep 0.2
        waited=$((waited + 1))
    done
    if kill -0 "$server_pid" 2>/dev/null; then
        kill "$server_pid" 2>/dev/null || true
        wait "$server_pid" 2>/dev/null
        server_rc="hung"
    else
        wait "$server_pid" 2>/dev/null
        server_rc=$?
    fi

    reason=""
    [ "$client_rc" -eq 0 ] || reason="${reason} client-exit=$client_rc"
    [ "$server_rc" = "0" ] || reason="${reason} server-exit=$server_rc"
    grep -q "^dp-server: received ${bytes} bytes$" "server-${label}.log" ||
        reason="${reason} server-did-not-receive-${bytes}-bytes"
    grep -q "^dp-server: verify: PASS$" "server-${label}.log" ||
        reason="${reason} server-verify-failed"

    # HOL checks: packet 4 (part of file 1) is always dropped, costing one
    # 250 ms timeout. With independent streams, file 3 must not wait for it.
    # With everything on one ordered stream, file 3 must wait for it.
    if [ -n "$hol_check" ]; then
        f3=$(sed -n 's/^dp-server: RESULT file=3 .*complete_ms=\([0-9.]*\).*/\1/p' "server-${label}.log")
        if [ -z "$f3" ]; then
            reason="${reason} no-file3-result"
        elif [ "$hol_check" = "fast" ]; then
            awk -v t="$f3" 'BEGIN { exit !(t < 100) }' ||
                reason="${reason} file3-blocked(${f3}ms,expected<100)"
        else
            awk -v t="$f3" 'BEGIN { exit !(t >= 200) }' ||
                reason="${reason} file3-not-blocked(${f3}ms,expected>=200)"
        fi
        [ -n "$f3" ] && echo "test: ${label} file3 complete_ms=${f3}"
    fi

    if [ -z "$reason" ]; then
        echo "test: ${label} PASS"
    else
        echo "test: ${label} FAIL:${reason}"
        failures=$((failures + 1))
    fi
    echo
}

#        label          bytes            server args                client args                            HOL
run_case clean-multi    $DEFAULT_BYTES   ""                         "-m multi"
run_case clean-single   $DEFAULT_BYTES   ""                         "-m single"
run_case loss10-multi   $DEFAULT_BYTES   "-l 0.10 -s 101"           "-m multi -l 0.10 -s 202"
run_case loss10-single  $DEFAULT_BYTES   "-l 0.10 -s 101"           "-m single -l 0.10 -s 202"
# 0-40 ms random delay on every packet: packets arrive out of order.
run_case reorder-multi  $DEFAULT_BYTES   "-d 40 -s 303"             "-m multi -d 40 -s 404"
run_case both20-single  $DEFAULT_BYTES   "-l 0.20 -d 20 -s 505"     "-m single -l 0.20 -d 20 -s 606"
# ACKs delayed 0-400 ms against a 250 ms timeout: retransmissions and
# duplicates with zero loss.
run_case slowack-multi  28672            "-d 400 -s 909"            "-m multi -k 16,8,4"
# Head-of-line blocking: the client always drops DATA packet 4.
run_case hol-multi      $DEFAULT_BYTES   ""                         "-m multi -D 4"                        fast
run_case hol-single     $DEFAULT_BYTES   ""                         "-m single -D 4"                       slow

if [ "$failures" -eq 0 ]; then
    echo "test: all reference tests PASS"
    exit 0
fi

echo "test: ${failures} test(s) FAILED"
echo "test: see client-*.log and server-*.log for details"
exit 1
