#!/usr/bin/env bash
# ============================================================================
#  build_godot.sh — fabrique le JEU SCPS (front Godot), REPRODUCTIBLE.
#
#  Le jeu jouable est le front Godot (carte parchemin, musique, panneaux). Ce
#  script compile la DLL du moteur (libscps, release), importe les assets, et
#  exporte l'exécutable Windows via les templates Godot.
#
#  Remplace build_windows.sh (qui packageait l'ancien VIEWER SDL, mort depuis
#  le 05/07/2026 — le viewer est désormais un outil console dev, pas le jeu).
#
#  Prérequis (Windows + MSYS2) :
#    - Godot 4.6.3 (binaire) + export templates 4.6.3 installés
#    - MSYS2/MinGW (scons, gcc) pour la DLL ; godot/godot-cpp (junction)
#    - À lancer depuis un shell MSYS2 MINGW64 :
#        MSYSTEM=MINGW64 /d/MSYS2/usr/bin/bash.exe -l packaging/windows/build_godot.sh
#
#  Sortie : packaging/windows/dist_godot/  (scps.exe + libscps…dll)
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
GODOT="${GODOT:-/e/JEUX/SCPS/Godot_v4.6.3-stable_win64.exe}"
PROJECT="$ROOT/godot/project"
DIST="$HERE/dist_godot"
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64

echo "→ 1/4  DLL du moteur (libscps, release)"
( cd "$ROOT/godot" && scons platform=windows use_mingw=yes target=template_release )

echo "→ 2/4  import des assets Godot"
"$GODOT" --headless --path "$PROJECT" --import

echo "→ 3/4  export « Windows Desktop » → scps.exe (PCK embarqué + libscps.dll)"
rm -rf "$DIST"; mkdir -p "$DIST"
"$GODOT" --headless --path "$PROJECT" --export-release "Windows Desktop" "$DIST/scps.exe"

echo "→ 4/4  bundle du LISEZMOI"
cp "$HERE/LISEZMOI.txt" "$DIST/" 2>/dev/null || true

echo
echo "✓ Jeu : $DIST/scps.exe"
ls -la "$DIST"
