#!/bin/sh

# Head-of-line blocking experiment.
#
# Sends the same three files (default 128, 32, 8 KB) in single-stream and
# multi-stream mode at several loss rates, several seeds each, and reports the
# average time at which each file finished at the server.
#
# Usage: ./hol.sh [seeds-per-point]      (default 5)
#        LOSSES="0 0.05" ./hol.sh 3       (choose loss rates)
# Output: printed table, also saved to hol-results.txt; raw logs in hol-*.log

SEEDS=${1:-5}
LOSSES=${LOSSES:-"0 0.01 0.02 0.05"}
RESULTS=hol-results.txt
tmp=hol-raw.txt
: > "$tmp"

run_one() {
    mode="$1"; loss="$2"; seed="$3"
    s_seed=$((seed * 2 + 1)); c_seed=$((seed * 2 + 2))
    log="hol-${mode}-${loss}-${seed}.log"
    ./dp-server -l "$loss" -s "$s_seed" >"$log" 2>&1 &
    pid=$!
    sleep 0.2
    ./dp-client -m "$mode" -l "$loss" -s "$c_seed" >/dev/null 2>&1
    wait "$pid"
    sed -n "s/^dp-server: RESULT file=\([0-9]*\) .*complete_ms=\([0-9.]*\).*/$mode $loss \1 \2/p" "$log" >>"$tmp"
}

for loss in $LOSSES; do
    for mode in single multi; do
        seed=1
        while [ "$seed" -le "$SEEDS" ]; do
            printf "hol: mode=%-6s loss=%-4s seed=%d\n" "$mode" "$loss" "$seed"
            run_one "$mode" "$loss" "$seed"
            seed=$((seed + 1))
        done
    done
done

{
    echo "Head-of-line blocking experiment: average completion time (ms) per file"
    echo "Files: 1=128KB 2=32KB 3=8KB, sent interleaved; ${SEEDS} seed(s) per row; loss applied both directions"
    echo
    awk -v losslist="$LOSSES" '
        { key = $1 " " $2; sum[key, $3] += $4; n[key, $3]++; keys[key] = 1 }
        END {
            printf "%-8s %-6s %10s %10s %10s\n", "loss", "mode", "file1", "file2", "file3"
            nl = split(losslist, losses, " ")
            split("single multi", modes, " ")
            for (i = 1; i <= nl; i++) for (j = 1; j <= 2; j++) {
                key = modes[j] " " losses[i]
                if (!(key in keys)) continue
                printf "%-8s %-6s", losses[i], modes[j]
                for (f = 1; f <= 3; f++)
                    printf " %10.1f", n[key, f] ? sum[key, f] / n[key, f] : -1
                printf "\n"
            }
        }' "$tmp"
} | tee "$RESULTS"
rm -f "$tmp"
