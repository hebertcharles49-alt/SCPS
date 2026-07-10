#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
make determinism 2>&1 | grep -iE "STABLE|IDENTIQUE|ÉCHEC|DIVERG|OK" | head -4
make golden 2>&1 | tail -1
