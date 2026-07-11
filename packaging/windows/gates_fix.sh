#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
make golden-update 2>&1 | tail -1
make determinism 2>&1 | grep -iE "STABLE|DIVERG" | head -1
cd godot && scons platform=windows use_mingw=yes target=template_debug -j4 2>&1 | tail -1 && scons platform=windows use_mingw=yes target=template_release -j4 2>&1 | tail -1
