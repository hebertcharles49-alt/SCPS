#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
echo "== make test =="
make test 2>&1 | tail -6
echo "== golden (attendu IDENTIQUE : lexique/NM/×12 sont display ou joueur-seuls) =="
make golden 2>&1 | tail -1
echo "== DLL =="
cd godot && scons platform=windows use_mingw=yes target=template_debug -j4 2>&1 | tail -1 && scons platform=windows use_mingw=yes target=template_release -j4 2>&1 | tail -1
