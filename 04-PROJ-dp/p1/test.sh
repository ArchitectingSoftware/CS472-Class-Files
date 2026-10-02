#!/bin/sh

# Runs every case, records each result, and returns non-zero only after all
# cases have been attempted. The scaffold is expected to fail until the TODOs
# are implemented.
#
# A case passes only when BOTH endpoints succeed:
#   - the client exits 0 (connect, send, and close all completed), and
#   - the server exits 0, reports exactly the expected byte count, and
#     verifies the byte pattern (every byte delivered once, in order).
# Checking only the client is not enough: a sender can believe it finished
# while the receiver is missing data.

failures=0
SERVER_WAIT_SECS=15

run_case() {
    label="$1"
    kb="$2"
    server_args="$3"
    client_args="$4"
    bytes=$((kb * 1024))

    printf "test: %s (%s KB)\n" "$label" "$kb"

    # Both endpoints use -v so their complete network traces can be merged later.
    ./dp-server $server_args -v >"server-${label}.log" 2>&1 &
    server_pid=$!
    sleep 0.2

    ./dp-client -v -k "$kb" $client_args >"client-${label}.log" 2>&1
    client_rc=$?

    # The server lingers briefly after CLOSE so it can answer a retransmitted
    # CLOSE. Let it exit on its own; kill it only if it hangs. If the client
    # already failed there is no point waiting long.
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
        if [ "$client_rc" -eq 0 ]; then server_rc="hung"; else server_rc="killed-after-client-failed"; fi
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

    if [ -z "$reason" ]; then
        echo "test: ${label} PASS"
    else
        echo "test: ${label} FAIL:${reason}"
        failures=$((failures + 1))
    fi
    echo
}

#        label    KB    server args                 client args
run_case clean    1024  ""                          ""
run_case loss10   64    "-l 0.10 -s 101"            "-l 0.10 -s 202"
run_case delay20  64    "-d 20 -s 303"              "-d 20 -s 404"
run_case both20   64    "-l 0.20 -d 20 -s 505"      "-l 0.20 -d 20 -s 606"
run_case loss50   8     "-l 0.50 -s 707"            "-l 0.50 -s 808"
# ACKs delayed 0-400 ms against a 250 ms timeout: timeouts and retransmissions
# with zero packet loss.
run_case slowack  8     "-d 400 -s 909"             ""

if [ "$failures" -eq 0 ]; then
    echo "test: all reference tests PASS"
    exit 0
fi

echo "test: ${failures} test(s) FAILED"
echo "test: see client-*.log and server-*.log for details"
exit 1
