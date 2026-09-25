#!/usr/bin/env bash
set -e

sim_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
make -C "$sim_dir"
exec "$sim_dir/hydrogen-sim"
