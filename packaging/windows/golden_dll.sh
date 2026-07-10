#!/usr/bin/env bash
# golden_dll.sh — make golden + rebuild des 2 DLL GDExtension (lancé en login shell MSYS2 ;
# le cd inline dans -lc se fait manger — piège documenté SYNTHESE — d'où CE script).
set -uo pipefail
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
echo "== make golden =="
make golden 2>&1 | tail -4
cd godot
scons platform=windows use_mingw=yes target=template_debug -j4 >/dev/null 2>&1 \
  && scons platform=windows use_mingw=yes target=template_release -j4 >/dev/null 2>&1
echo "DLL_EXIT=$?"
