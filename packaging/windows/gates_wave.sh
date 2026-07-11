#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
echo "== make test =="
make test 2>&1 | tail -7
echo "== golden-update (les âges mordent an-0) =="
make golden-update 2>&1 | tail -1
echo "== determinism =="
make determinism 2>&1 | grep -iE "STABLE|ÉCHEC|DIVERG" | head -2
echo "== savetest v78 (graines 9 et 7) =="
make scps >/dev/null 2>&1 && ./scps_viewer --savetest 9 2>&1 | tail -1 && ./scps_viewer --savetest 7 2>&1 | tail -1
