#!/usr/bin/env bash
set -uo pipefail
cd /c/Users/Charl/Desktop/SCPS-main
make ai_demo 2>&1 | tail -2 && ./ai_demo 2>&1 | tail -2 && make smoke 2>&1 | tail -3
