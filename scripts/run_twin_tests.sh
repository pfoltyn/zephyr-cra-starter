#!/usr/bin/env bash
# Run the Renode twin test suite against an existing build.
#
# Usage: zephyr-cra-starter/scripts/run_twin_tests.sh [build_dir]
# Needs `renode-test` on PATH (or $RENODE_TEST) and Renode's tests/requirements.txt installed.
#
# SPDX-License-Identifier: Apache-2.0
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
WS="$(cd "$REPO/.." && pwd)"
BUILD="${1:-$WS/build}"
case "$BUILD" in /*) ;; *) BUILD="$PWD/$BUILD" ;; esac
RENODE_TEST="${RENODE_TEST:-renode-test}"

RESULTS="$BUILD/twin-results"
mkdir -p "$RESULTS"
cd "$RESULTS"
"$RENODE_TEST" --variable "BUILD:$BUILD" -r "$RESULTS" "$REPO"/tests/twin/*.robot
