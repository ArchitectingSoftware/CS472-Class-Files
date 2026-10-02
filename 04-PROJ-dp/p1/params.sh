#!/bin/sh
# Personalized experiment parameters for Part 1.
#
# Usage: ./params.sh DREXEL_ID        (your Drexel username, e.g. abc123)
#
# Every student gets their own values, derived from their Drexel ID. Your
# interpretation answers must match the evidence produced with YOUR values.

id=$(printf '%s' "${1:-}" | tr 'A-Z' 'a-z')
if [ -z "$id" ]; then
    echo "usage: $0 DREXEL_ID   (for example: $0 abc123)" >&2
    exit 1
fi

h=$(printf '%s' "$id" | cksum | cut -d' ' -f1)

DATA_DROP=$((3 + h % 38))                    # client drops this DATA packet once: 3..40
ACK_DROP=$((3 + (h / 38) % 38))              # server drops the ACK for this packet once: 3..40
[ "$ACK_DROP" -eq "$DATA_DROP" ] && ACK_DROP=$((ACK_DROP % 38 + 3))
SIZE_KB=$((32 + 8 * ((h / 1444) % 9)))       # transfer size: 32..96 KB
DELAY=$((10 + 5 * ((h / 12996) % 5)))        # max random delay per direction: 10..30 ms

echo "DREXEL_ID=$id"
echo "DATA_DROP=$DATA_DROP"
echo "ACK_DROP=$ACK_DROP"
echo "SIZE_KB=$SIZE_KB"
echo "DELAY=$DELAY"
