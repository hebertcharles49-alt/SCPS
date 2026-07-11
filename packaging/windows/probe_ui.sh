#!/usr/bin/env bash
# Probe UI FENÊTRÉE (docs/UI_RECO_2026-07-10.md §3.4) — capture shot_ui à une résolution.
# Usage : probe_ui.sh 1280x720   (défaut 1920x1080). Sortie : godot/project/shots_ui/<WxH>/
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
RES="${1:-1920x1080}"
GODOT="${GODOT:-/e/JEUX/SCPS/Godot_v4.6.3-stable_win64.exe}"
rm -f godot/project/session_running.flag 2>/dev/null
echo "== probe $RES =="
"$GODOT" --path godot/project res://shot_ui.tscn -- seed=9 years=25 "res=$RES" 2>&1 | grep -iE "SHOT|SHOTS OK|error|SCRIPT ERROR|crash|no world" | head -40
echo "== fichiers =="
ls -1 "godot/project/shots_ui/$RES/" 2>/dev/null | wc -l
