#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
echo "== econ_tax_demo =="
make econ_tax_demo 2>&1 | grep -oE "undefined reference to .[a-z_]+" | sort -u | head -6
echo "== statecraft_demo (rouge) =="
make statecraft_demo >/dev/null 2>&1; ./statecraft_demo 2>&1 | grep -B1 "✗" | head -8
