#!/usr/bin/env bash
set -u
# ═══════════════════════════════════════════════════════════════════════════
#  SWEEP APPARIÉ 3×3 — « L'IA ADOPTE-T-ELLE DES DOCTRINES SANS CASSER LE MONDE ? »
#  (P3-IA, docs/BRIEF_P3_IA_CHRONICLE.md §3 — préparé par l'agent, LANCÉ PAR LE
#   JOUEUR : règle de projet FERME, un balayage ne se déclenche jamais tout seul.)
#
#  DEUX BRAS APPARIÉS, mêmes graines, même monde, un seul tunable de différence :
#     témoin  : SCPS_TUNE=AI_DOCT=0   (l'IA n'adopte JAMAIS — la trajectoire golden)
#     essai   : (défaut, AI_DOCT=1)   (l'IA joue l'arbre)
#  3 GRAINES × 3 RÉPÉTITIONS (chronicle avance la graine de 101 entre ses sims)
#  × 2 HORIZONS (an 120 et an 180 : le bilan de fin de sim EST la mesure) ×
#  2 BRAS = 36 simulations. Plafond de projet 30-40 sims : GIGA (100) sur demande
#  explicite SEULEMENT.
#
#  USAGE :  bash tools/sweep_doct_ai.sh [dossier_de_sortie]
#  ENV   :  JOBS=4 (parallélisme)  ·  SEEDS="7 1009 4243" (les 3 graines)
#
#  CE QU'ON LIT (résumé auto en fin de course, détail dans les .log) :
#    · pop mondiale · indice de prix · guerres · trésors  → l'équilibre tient-il ?
#    · distribution des doctrines · influence méd/max     → l'arbre vit-il ?
#    · corrélations-juges (côtiers→Colonisation,
#      suzerains→Vassaux, belligérants→Offense/Défense)   → le SCORE choisit-il ?
#  Un bras témoin qui montre une distribution NON VIDE serait un BUG (le
#  kill-switch fuit) : le résumé le signale.
# ═══════════════════════════════════════════════════════════════════════════

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-sweep_doct_ai_$(date -u +%Y%m%d)}"
JOBS="${JOBS:-4}"
SEEDS="${SEEDS:-7 1009 4243}"
EMPIRES="${EMPIRES:-6}"
CITIES="${CITIES:-12}"
REPS=3            # sims par invocation (les « 3 » du 3×3)

cd "$ROOT" || exit 1
if [ ! -x ./chronicle ]; then
    echo "ERREUR: ./chronicle est absent ou non executable (make chronicle)" >&2
    exit 2
fi
mkdir -p "$OUT"
if find "$OUT" -maxdepth 1 -name '*.log' -print -quit | grep -q .; then
    echo "ERREUR: $OUT contient deja des journaux" >&2
    exit 3
fi

{
    echo "protocol=sweep-doct-ai-paired-3x3-v1"
    echo "started_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "seeds=$SEEDS"
    echo "reps_per_seed=$REPS"
    echo "horizons=120 180"
    echo "arms=temoin(AI_DOCT=0) essai(AI_DOCT=1)"
    echo "empires=$EMPIRES city_states=$CITIES"
    echo "total_sims=$(( $(echo $SEEDS | wc -w) * REPS * 2 * 2 ))"
    sha256sum ./chronicle 2>/dev/null || true
} > "$OUT/manifest.txt"

run_one(){   # $1=bras  $2=graine  $3=années
    local arm="$1" seed="$2" yr="$3"
    local log="$OUT/${arm}_s${seed}_y${yr}.log"
    if [ "$arm" = "temoin" ]; then
        SCPS_TUNE=AI_DOCT=0 ./chronicle "$seed" "$REPS" "$yr" "$EMPIRES" "$CITIES" > "$log" 2>&1
    else
        ./chronicle "$seed" "$REPS" "$yr" "$EMPIRES" "$CITIES" > "$log" 2>&1
    fi
    printf '%s\n' "$?" > "$OUT/${arm}_s${seed}_y${yr}.rc"
}

running=0
for yr in 120 180; do
  for seed in $SEEDS; do
    for arm in temoin essai; do
        run_one "$arm" "$seed" "$yr" &
        running=$((running+1))
        if [ "$running" -ge "$JOBS" ]; then wait -n; running=$((running-1)); fi
    done
  done
done
while [ "$running" -gt 0 ]; do wait -n; running=$((running-1)); done

# ── LE RÉSUMÉ APPARIÉ ──────────────────────────────────────────────────────
sum_arm(){   # $1=bras $2=années → une ligne de moyennes sur les 9 sims du bras
    local arm="$1" yr="$2"
    cat "$OUT/${arm}"_s*_y"${yr}".log 2>/dev/null | awk -v arm="$arm" -v yr="$yr" '
      /BILAN an /                { np+=$5;  nn++ }
      /guerre\(s\) au total/     { for(i=1;i<=NF;i++) if($i=="guerre(s)"){ w+=$(i-1); wn++ } }
      /masse monétaire/          { for(i=1;i<=NF;i++) if($i ~ /^M\(fin\)=/){ split($i,a,"="); m+=a[2]; mn++ } }
      /inflation \(M7-I1\)/      { for(i=1;i<=NF;i++) if($i=="moy"){ p+=$(i+1); pn++ } }
      /DOCTRINES \(P3-IA\)/      { for(i=1;i<=NF;i++) if($i=="doctrines" && $(i+1)=="actives"){ d+=$(i-1); dn++ } }
      /corrélations-juges/       { cj++ ;
                                   for(i=1;i<=NF;i++){
                                     if($i=="côtiers→Colonisation"){ split($(i+2),a,"[()%]"); c1+=a[2]; c1n++ }
                                     if($i=="suzerains→Vassaux"){    split($(i+2),a,"[()%]"); c2+=a[2]; c2n++ }
                                     if($i=="belligérants→Offense/Défense"){ split($(i+2),a,"[()%]"); c3+=a[2]; c3n++ }
                                   } }
      END {
        printf "  %-7s an %3s | pays %5.1f | guerres %6.1f | M(fin) %10.0f | prix %5.3f | doctrines %5.1f | juges côte %4.0f%% vassal %4.0f%% guerre %4.0f%%\n",
          arm, yr, (nn?np/nn:0), (wn?w/wn:0), (mn?m/mn:0), (pn?p/pn:0), (dn?d/dn:0),
          (c1n?c1/c1n:0), (c2n?c2/c2n:0), (c3n?c3/c3n:0);
      }'
}

{
    echo "── SWEEP APPARIÉ AI_DOCT (3 graines × $REPS répétitions × 2 horizons) ──"
    for yr in 120 180; do sum_arm temoin "$yr"; sum_arm essai "$yr"; done
    echo
    echo "── distribution des doctrines (bras ESSAI, toutes sims confondues) ──"
    grep -h "distribution :" "$OUT"/essai_*.log 2>/dev/null \
      | sed 's/^ *distribution ://' | tr '·' '\n' \
      | awk 'NF==2 { n[$1]+=$2 } END { for (k in n) printf "  %-16s %d\n", k, n[k] }' | sort -k2 -nr
    echo
    echo "── contrôle du kill-switch (le bras TÉMOIN doit être MUET) ──"
    if grep -qh "^   DOCTRINES (P3-IA) : [1-9]" "$OUT"/temoin_*.log 2>/dev/null; then
        echo "  ⚠ FUITE : un pays a adopté une doctrine avec AI_DOCT=0 — BUG."
    else
        echo "  OK : aucune adoption dans le bras témoin."
    fi
} > "$OUT/resume.txt" 2>&1

cat "$OUT/resume.txt"
echo "finished_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)" >> "$OUT/manifest.txt"
nonzero=$(grep -L '^0$' "$OUT"/*.rc 2>/dev/null | wc -l)
echo "nonzero_runs=$nonzero" >> "$OUT/manifest.txt"
[ "$nonzero" -eq 0 ] || exit 4
exit 0
