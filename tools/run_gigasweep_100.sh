#!/usr/bin/env bash
set -u

# ⚠ GIGASWEEP — 100 SIMULATIONS. NE PAS LANCER SANS ACCORD EXPLICITE DU JOUEUR.
# Règle de projet FERME : un balayage GIGA (100 sims) est coûteux en CPU et ne se
# lance QUE sur demande explicite. Le standard des gates est le balayage APPARIÉ
# (~30-40 sims max) ; ce script est un OUTIL d'audit ponctuel, pas une étape de
# routine. Lancé par erreur, il monopolise la machine des minutes durant.

# Balayage reproductible de 100 mondes distincts :
#   graines 3 + 101*n, n=0..99
# Chronicle avance lui-même la graine de 101 entre les simulations d'un lot.

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-sweep_giga_2026-07-19_100}"
JOBS="${JOBS:-4}"
YEARS="${YEARS:-250}"

cd "$ROOT" || exit 1

if [ ! -x ./chronicle ]; then
    echo "ERREUR: ./chronicle est absent ou non executable" >&2
    exit 2
fi

mkdir -p "$OUT"
if find "$OUT" -maxdepth 1 -name 'seed_*.log' -print -quit | grep -q .; then
    echo "ERREUR: $OUT contient deja des journaux seed_*.log" >&2
    exit 3
fi

{
    echo "protocol=gigasweep-100-disjoint-v1"
    echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "worlds=100"
    echo "years=$YEARS"
    echo "empires=6"
    echo "city_states=12"
    echo "parallel_jobs=$JOBS"
    echo "seed_formula=3+101*n,n=0..99"
    sha256sum ./chronicle 2>/dev/null || true
} > "$OUT/manifest.txt"

run_batch() {
    local base="$1"
    local log="$OUT/seed_${base}.log"
    local rc_file="$OUT/seed_${base}.rc"

    ./chronicle "$base" 5 "$YEARS" 6 12 > "$log" 2>&1
    local rc=$?
    printf '%s\n' "$rc" > "$rc_file"
    return "$rc"
}

running=0
failed=0
for ((i=0; i<20; i++)); do
    base=$((3 + 505*i))
    run_batch "$base" &
    running=$((running + 1))

    if [ "$running" -ge "$JOBS" ]; then
        if ! wait -n; then
            failed=$((failed + 1))
        fi
        running=$((running - 1))
    fi
done

while [ "$running" -gt 0 ]; do
    if ! wait -n; then
        failed=$((failed + 1))
    fi
    running=$((running - 1))
done

rc_count=$(find "$OUT" -maxdepth 1 -name 'seed_*.rc' | wc -l)
sim_count=$(grep -h -c '^── Sim ' "$OUT"/seed_*.log 2>/dev/null | awk '{s += $1} END {print s+0}')
nonzero=$(grep -L '^0$' "$OUT"/seed_*.rc 2>/dev/null | wc -l)

{
    echo "finished_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "batch_rc_files=$rc_count"
    echo "simulations_detected=$sim_count"
    echo "failed_waits=$failed"
    echo "nonzero_batches=$nonzero"
} >> "$OUT/manifest.txt"

python tools/sweep_analyze.py "$OUT" > "$OUT/analysis.txt" 2>&1
analysis_rc=$?
printf 'analysis_rc=%s\n' "$analysis_rc" >> "$OUT/manifest.txt"

if [ "$rc_count" -ne 20 ] || [ "$sim_count" -ne 100 ] || [ "$nonzero" -ne 0 ] || [ "$analysis_rc" -ne 0 ]; then
    exit 4
fi

exit 0
