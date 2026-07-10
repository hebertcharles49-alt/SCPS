#!/usr/bin/env bash
set -uo pipefail
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main/godot
scons platform=windows use_mingw=yes target=template_debug -j4 2>&1 | tail -3
scons platform=windows use_mingw=yes target=template_release -j4 2>&1 | tail -3
ls -la project/bin/*.dll 2>/dev/null || ls -la bin/*.dll 2>/dev/null
