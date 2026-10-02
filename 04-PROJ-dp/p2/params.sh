#!/bin/sh
# Personalized experiment parameters.
#
# Usage: ./params.sh DREXEL_ID        (your Drexel username, e.g. abc123)
#
# Every student gets a different forced-drop packet number and different file
# sizes, derived from their Drexel ID. Your interpretation answers must match
# the evidence produced with YOUR parameters.

id=$(printf '%s' "${1:-}" | tr 'A-Z' 'a-z')
if [ -z "$id" ]; then
    echo "usage: $0 DREXEL_ID   (for example: $0 abc123)" >&2
    exit 1
fi

h=$(printf '%s' "$id" | cksum | cut -d' ' -f1)

# DROP: a DATA packet number from 4 to 27. All three files are still being
# interleaved at that point, so the dropped chunk can belong to any file.
DROP=$((4 + h % 24))
K1=$((64 + 4 * ((h / 24) % 25)))     # file 1: 64..160 KB
K2=$((16 + 4 * ((h / 600) % 9)))     # file 2: 16..48 KB
K3=$((4 + (h / 5400) % 9))           # file 3: 4..12 KB

echo "DREXEL_ID=$id"
echo "DROP=$DROP"
echo "SIZES=$K1,$K2,$K3"
