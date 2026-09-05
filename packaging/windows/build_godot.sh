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
#  Sortie : packaging/windows/dist_godot/staging-<UTC>/  (scps.exe + libscps…dll)
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
GODOT="${GODOT:-${GODOT_BIN:-}}"
if [ -z "$GODOT" ]; then
  for candidate in "$(command -v godot4 2>/dev/null || true)" "$(command -v godot 2>/dev/null || true)" \
      "/c/Program Files/Godot/Godot_v4.6.3-stable_win64.exe" "/c/Program Files/Godot/Godot.exe"; do
    if [ -n "$candidate" ] && [ -f "$candidate" ]; then GODOT="$candidate"; break; fi
  done
fi
[ -n "$GODOT" ] && [ -f "$GODOT" ] || { echo "ERREUR : Godot introuvable ; définir GODOT_BIN" >&2; exit 2; }
PROJECT="$ROOT/godot/project"
DIST="${DIST:-$HERE/dist_godot/staging-$(date -u +%Y%m%dT%H%M%SZ)}"
# Les programmes Windows lisent TEMP comme un chemin natif. /tmp pointerait
# vers le répertoire MSYS, parfois non accessible (export project.binary vide).
( cd "$ROOT" && mkdir -p build/packaging-tmp )
TMP="$(cygpath -w "$ROOT/build/packaging-tmp")"
export TMP TEMP="$TMP" TMPDIR="$TMP" PROCESSOR_ARCHITECTURE=AMD64
# Godot lit les export templates dans %APPDATA%\Godot ; un shell MSYS2 login ne propage pas
# toujours APPDATA → Godot chercherait un ./Godot LOCAL vide et l'export ÉCHOUERAIT
# (« aucun modèle d'exportation trouvé »). On le restaure depuis cmd.exe si besoin ;
# Si l'environnement est scrubé et que cmd.exe n'est pas disponible, l'appelant
# doit fournir APPDATA explicitement : on ne choisit jamais le profil d'un autre
# utilisateur par balayage du disque.
[ -z "${APPDATA:-}" ] && export APPDATA="$(cmd /c 'echo %APPDATA%' 2>/dev/null | tr -d '\r')"
GODOT_VERSION="$("$GODOT" --version 2>/dev/null || true)"
echo "$GODOT_VERSION" | grep -qE '^4\.6\.3([-.]|$)' || {
  echo "ERREUR : Godot 4.6.3 requis (trouvé: ${GODOT_VERSION:-inconnu})" >&2; exit 2;
}
[ ! -e "$DIST" ] || { echo "ERREUR : destination déjà existante : $DIST" >&2; exit 2; }

echo "→ 1/4  DLL du moteur (libscps, release)"
( cd "$ROOT/godot" && scons platform=windows use_mingw=yes target=template_release )

echo "→ 2/4  import des assets Godot"
"$GODOT" --headless --path "$PROJECT" --import

echo "→ 3/4  export « Windows Desktop » → scps.exe (PCK embarqué + libscps.dll)"
mkdir -p "$DIST"
# Godot interprète un chemin relatif par rapport au projet, alors que mkdir
# l'a créé depuis le shell appelant. Fixer une destination absolue unique.
DIST="$(cd "$DIST" && pwd)"
"$GODOT" --headless --path "$PROJECT" --export-release "Windows Desktop" "$DIST/scps.exe"

echo "→ 4/4  bundle du LISEZMOI"
cp "$HERE/LISEZMOI.txt" "$DIST/"

echo "→ manifeste SHA-256, version et état du dépôt"
{
  echo "format=scps-windows-package-v2"
  echo "built_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "godot_version=$GODOT_VERSION"
  echo "git_commit=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
  echo "git_version=$(git -C "$ROOT" describe --always --dirty 2>/dev/null || echo unknown)"
  if [ -z "$(git -C "$ROOT" status --porcelain --untracked-files=all 2>/dev/null)" ]; then echo "git_dirty=false"; else echo "git_dirty=true"; fi
} > "$DIST/MANIFEST-SHA256.txt"
(
  cd "$DIST"
  find . -type f ! -name MANIFEST-SHA256.txt -print0 | sort -z |
    while IFS= read -r -d '' f; do sha256sum "$f"; done
) >> "$DIST/MANIFEST-SHA256.txt"

echo
echo "✓ Jeu : $DIST/scps.exe"
ls -la "$DIST"
