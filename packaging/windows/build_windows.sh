#!/usr/bin/env bash
# ============================================================================
#  build_windows.sh — fabrique l'installateur Windows de SCPS, REPRODUCTIBLE.
#
#  Cross-compile (Linux → Windows x64, MinGW-w64) le visualiseur SDL2 et les
#  outils headless, bundle les DLL SDL2 + la police, et empaquette en
#  SCPS-Setup.exe (NSIS). Aucun runtime tiers requis chez l'utilisateur.
#
#  Prérequis (Debian/Ubuntu) :
#    sudo apt-get install -y mingw-w64 nsis curl
#  (Linux libsdl2-dev N'EST PAS utilisé : on récupère les libs MinGW de SDL.)
#
#  Usage :  packaging/windows/build_windows.sh
#  Sortie : packaging/windows/out/SCPS-Setup.exe (+ dist/ déballé)
# ============================================================================
set -euo pipefail

# --- versions des dépendances vendorées (épinglées) -------------------------
SDL2_VER=2.30.9
TTF_VER=2.22.0
HOST=x86_64-w64-mingw32
CC=${CC:-${HOST}-gcc}
STRIP=${STRIP:-${HOST}-strip}

# --- arborescence -----------------------------------------------------------
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"          # racine du dépôt SCPS
WORK="$HERE/.work"                          # téléchargements + extraction SDL
DIST="$HERE/dist"                           # binaires + DLL prêts à empaqueter
OUT="$HERE/out"                             # SCPS-Setup.exe
mkdir -p "$WORK" "$OUT"
rm -rf "$DIST"; mkdir -p "$DIST"

# --- 1. SDL2 + SDL2_ttf (libs de développement MinGW) -----------------------
fetch() { # url sha-not-checked-here(épinglé par version) dest
  local url="$1" dst="$2"
  [ -f "$dst" ] || { echo "↓ $(basename "$dst")"; curl -fsSL -o "$dst" "$url"; }
}
SDL2_TGZ="$WORK/SDL2-devel-${SDL2_VER}-mingw.tar.gz"
TTF_TGZ="$WORK/SDL2_ttf-devel-${TTF_VER}-mingw.tar.gz"
fetch "https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VER}/SDL2-devel-${SDL2_VER}-mingw.tar.gz" "$SDL2_TGZ"
fetch "https://github.com/libsdl-org/SDL_ttf/releases/download/release-${TTF_VER}/SDL2_ttf-devel-${TTF_VER}-mingw.tar.gz" "$TTF_TGZ"
tar -C "$WORK" -xzf "$SDL2_TGZ"
tar -C "$WORK" -xzf "$TTF_TGZ"
S="$WORK/SDL2-${SDL2_VER}/${HOST}"
T="$WORK/SDL2_ttf-${TTF_VER}/${HOST}"

# --- 2. cross-compilation ---------------------------------------------------
SDL_CFLAGS="-I$S/include -I$S/include/SDL2 -I$T/include/SDL2 -Dmain=SDL_main"
SDL_LIBS="-L$S/lib -L$T/lib -lmingw32 -lSDL2main -lSDL2"

cd "$ROOT"
make clean >/dev/null 2>&1 || true
echo "→ outils headless (sans SDL : -Dmain ne doit PAS renommer leur main)"
make chronicle core_demo WIN=1 CC="$CC"
echo "→ visualiseur SDL2 (avec -Dmain=SDL_main + les libs MinGW)"
make scps WIN=1 CC="$CC" SDL_CFLAGS="$SDL_CFLAGS" SDL_LIBS="$SDL_LIBS"

# --- 3. assemblage de la distribution --------------------------------------
cp scps_viewer.exe chronicle.exe core_demo.exe "$DIST/"
cp "$S/bin/SDL2.dll" "$T/bin/SDL2_ttf.dll" "$DIST/"
# Police bundlée (DejaVu couvre les accents FR) — repli système C:\Windows\Fonts\arial.ttf.
for f in /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf /usr/share/fonts/dejavu/DejaVuSans.ttf; do
  [ -f "$f" ] && { cp "$f" "$DIST/"; break; }
done
cp "$HERE/LISEZMOI.txt" "$DIST/"
"$STRIP" "$DIST"/*.exe "$DIST"/*.dll 2>/dev/null || true

# --- 4. empaquetage NSIS ----------------------------------------------------
cp "$HERE/installer.nsi" "$DIST/installer.nsi"
( cd "$DIST" && makensis -V2 -DOUTFILE="$OUT/SCPS-Setup.exe" installer.nsi )

echo
echo "✓ Installateur : $OUT/SCPS-Setup.exe"
ls -la "$OUT/SCPS-Setup.exe"
make clean >/dev/null 2>&1 || true   # ne pas laisser d'objets Windows dans build/
