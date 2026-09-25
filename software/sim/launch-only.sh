#!/usr/bin/env bash
set -e

sim_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
binary="$sim_dir/hydrogen-sim"

if [[ ! -x "$binary" ]]; then
    echo "HYDROGEN simulator is not built yet." >&2
    echo "Run compile-and-launch.sh first." >&2
    exit 1
fi

exec "$binary"
