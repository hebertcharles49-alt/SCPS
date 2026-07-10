#!/usr/bin/env bash
# gates_tech.sh — gates de la réécriture des techs (2026-07-10).
# Usage : MSYSTEM=MINGW64 D:\MSYS2\usr\bin\bash.exe -l <ce script>
# (le cd vit ICI — l'inline « bash -lc 'cd … && …' » perd son cd, piège documenté)
set -uo pipefail
cd /c/Users/Charl/Desktop/SCPS-main

echo "== tech_demo =="
make tech_demo 2>&1 | tail -2
./tech_demo 2>&1 | tail -2
echo "== lang-check =="
make lang-check 2>&1 | tail -2
echo "== smoke =="
make smoke 2>&1 | tail -4
echo "== golden =="
make golden 2>&1 | tail -8
echo "== determinism =="
make determinism 2>&1 | tail -4
