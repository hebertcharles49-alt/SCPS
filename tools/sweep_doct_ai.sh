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
#    · pays · guerres · masse monétaire · indice · GRAIN   → l'équilibre tient-il ?
#    · distribution des doctrines · influence méd/max      → l'arbre vit-il ?
#    · corrélations-juges ADOPTANTS (côtiers→Colonisation,
#      suzerains→Vassaux, belligérants→Offense/Défense)    → le SCORE choisit-il ?
#    · rgt/limite · désertions · sur-budget · décrochages  → la vague W1 tient-elle ?
#    · LEDGERS figées % · fidèles (porte du Divin)
#  Un bras témoin qui montre une distribution NON VIDE serait un BUG (le
#  kill-switch fuit) : le résumé le signale.
#
#  ⚠ DEUX COLONNES DE PRIX, ET ELLES NE DISENT PAS LA MÊME CHOSE (2026-09-03) :
#    · « indice » = `inflation (M7-I1) : indice moy` — un RAPPORT caisse/valeur ajoutée
#      (le niveau de prix d'un empire), PAS un prix. La colonne s'appelait « prix » :
#      elle mentait.
#    · « grain »  = `prix du grain : médiane` — le prix RÉEL du pain, médiane sur les
#      provinces tenues, en regard de sa base (1.00). C'est LUI qu'on lit pour savoir
#      si le pain est cher.
#  Le résumé est un INDEX, pas le rapport : l'analyste LIT les .log (chaque sim y
#  publie ses lignes complètes), il ne compte pas des grep.
# ═══════════════════════════════════════════════════════════════════════════

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-sweep_doct_ai_$(date -u +%Y%m%d)}"
JOBS="${JOBS:-4}"
SEEDS="${SEEDS:-7 1009 4243}"
EMPIRES="${EMPIRES:-6}"
CITIES="${CITIES:-12}"
REPS="${REPS:-3}"                 # sims par invocation (les « 3 » du 3×3 ; 1 = une sim par graine)
HORIZONS="${HORIZONS:-120 180}"   # années de fin (le bilan de fin de sim EST la mesure)
# (2026-09-02, commande joueur « sweep 10 graines 200 ans » : SEEDS/REPS/HORIZONS
#  passés en env — le protocole 3×3 reste le défaut.)

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
    echo "horizons=$HORIZONS"
    echo "arms=temoin(AI_DOCT=0) essai(AI_DOCT=1)"
    echo "empires=$EMPIRES city_states=$CITIES"
    echo "total_sims=$(( $(echo $SEEDS | wc -w) * REPS * $(echo $HORIZONS | wc -w) * 2 ))"
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
for yr in $HORIZONS; do
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
sum_arm(){   # $1=bras $2=années → DEUX lignes de moyennes sur toutes les sims du bras
    local arm="$1" yr="$2"
    cat "$OUT/${arm}"_s*_y"${yr}".log 2>/dev/null | awk -v arm="$arm" -v yr="$yr" '
      # grab(re) : rend le NOMBRE qui TERMINE la portion de ligne appariée par `re`
      # (POSIX : match/RSTART/RLENGTH + substr — pas de match() à 3 arguments gawk).
      # Chaque motif doit donc se terminer par le nombre voulu.
      function grab(re,   s){
        if (!match($0, re)) return "";
        s = substr($0, RSTART, RLENGTH);
        sub(/^.*[^0-9.+-]/, "", s);
        return s;
      }
      # grab1 : le nombre de TÊTE de la portion appariée (quand le motif ne peut pas se
      # terminer par lui — « 22 décrochage(s) », le mot vient APRÈS le nombre).
      function grab1(re,   s){
        if (!match($0, re)) return "";
        s = substr($0, RSTART, RLENGTH);
        sub(/[^0-9.+-].*$/, "", s);
        return s;
      }
      # ⚠ awk : AUCUN espace entre le nom d une fonction et sa parenthèse ouvrante
      # (« acc (…) » est une erreur de syntaxe gawk, pas un avertissement).
      function acc(re, key,   v){ v=grab(re);  if (v!=""){ S[key]+=v+0; N[key]++ } }
      function acc1(re, key,   v){ v=grab1(re); if (v!=""){ S[key]+=v+0; N[key]++ } }
      function avg(k){ return (N[k]? S[k]/N[k] : 0) }

      /BILAN an /                { np+=$5;  nn++ }
      /guerre\(s\) au total/     { for(i=1;i<=NF;i++) if($i=="guerre(s)"){ w+=$(i-1); wn++ } }
      /masse monétaire/          { for(i=1;i<=NF;i++) if($i ~ /^M\(fin\)=/){ split($i,a,"="); m+=a[2]; mn++ } }
      /inflation \(M7-I1\)/      { acc("indice moy [0-9.]+", "indice") }
      /^   prix du grain :/      { acc("médiane [0-9.]+",    "grain")  }
      /DOCTRINES \(P3-IA\)/      { for(i=1;i<=NF;i++) if($i=="doctrines" && $(i+1)=="actives"){ d+=$(i-1); dn++ } }
      # LES JUGES : la ligne ADOPTANTS SEULS (« la vraie mesure du départage », 2026-09-02).
      # La ligne non-ADOPTANTS reste dans le .log pour qui veut les deux.
      /corrélations-juges \(ADOPTANTS/ {
                                   cj++ ;
                                   for(i=1;i<=NF;i++){
                                     if($i=="côtiers→Colonisation"){ split($(i+2),a,"[()%]"); c1+=a[2]; c1n++ }
                                     if($i=="suzerains→Vassaux"){    split($(i+2),a,"[()%]"); c2+=a[2]; c2n++ }
                                     if($i=="belligérants→Offense/Défense"){ split($(i+2),a,"[()%]"); c3+=a[2]; c3n++ }
                                   } }
      # ── VAGUE W1 : ce que la vague a changé, une colonne par chantier ──────────
      # ⚠ les motifs passés à acc/acc1 sont des CHAÎNES : une constante /re/ en argument
      # de fonction vaut « $0 ~ /re/ » en gawk — un booléen, pas un motif. Parenthèses
      # écrites [(] car dans une chaîne « \( » ne serait pas un échappement valide.
      # ⚠ AUCUNE apostrophe dans ce bloc : il vit dans un awk à quotes SIMPLES — une
      # apostrophe de commentaire ferme la chaîne shell et casse tout le script.
      /armée \/ limite de force/ { acc("médiane [0-9]+",               "rgtfl") }
      /solde \/ revenu fiscal/   { acc("médiane [0-9]+",               "payrev") }
      /^              batailles :/ { acc1("[0-9]+ décrochage",         "decroch") }
      /^              frein de levée :/ { acc(": [0-9]+",              "desert")
                                          acc("sur-budget [(][0-9]+",  "surbud") }
      /LEDGERS \(P11\)/          { acc("journaliers [(][0-9]+",        "figees") }
      /foi d.État \(assiette Divin\)/ { acc("fidèles Σ [0-9]+",        "fideles") }
      /foi d.État \(porte Divin\)/    { acc("religion d.État [(][0-9]+","portefoi") }
      END {
        printf "  %-7s an %3s | pays %5.1f | guerres %6.1f | M(fin) %10.0f | indice %5.3f | grain %6.3f | doctrines %5.1f | juges côte %4.0f%% vassal %4.0f%% guerre %4.0f%%\n",
          arm, yr, (nn?np/nn:0), (wn?w/wn:0), (mn?m/mn:0), avg("indice"), avg("grain"), (dn?d/dn:0),
          (c1n?c1/c1n:0), (c2n?c2/c2n:0), (c3n?c3/c3n:0);
        printf "  %-7s   W1 | rgt/limite %3.0f%% | solde/revenu %3.0f%% | désert %6.1f rgt | sur-budget %3.0f%% | décroch %5.1f | LEDGERS figées %3.0f%% | fidèles %7.0f (porte %3.0f%%)\n",
          arm, avg("rgtfl"), avg("payrev"), avg("desert"), avg("surbud"), avg("decroch"),
          avg("figees"), avg("fideles"), avg("portefoi");
      }'
}

{
    echo "── SWEEP APPARIÉ AI_DOCT ($(echo $SEEDS | wc -w) graines × $REPS répétition(s) × horizons $HORIZONS) ──"
    echo "   indice = niveau de prix (caisse/VA, M7-I1) · grain = PRIX du pain, médiane, base 1.00"
    echo "   rgt/limite = armée vs limite de force · solde/revenu = ce que la solde mange du revenu"
    echo "   désert = régiments partis faute de solde · sur-budget = mois-pays au-dessus du plafond"
    echo "   figées = provinces 100 % journaliers (LEDGERS P11) · porte = empires à religion d'État"
    for yr in $HORIZONS; do sum_arm temoin "$yr"; sum_arm essai "$yr"; done
    echo
    echo "── distribution des doctrines (bras ESSAI, toutes sims confondues) ──"
    grep -h "distribution :" "$OUT"/essai_*.log 2>/dev/null \
      | sed 's/^ *distribution ://' | tr '·' '\n' \
      | awk 'NF==2 { n[$1]+=$2 } END { for (k in n) printf "  %-16s %d\n", k, n[k] }' | sort -k2 -nr
    echo
    echo "── runs en ÉCHEC (code de retour ≠ 0 : banc invariant M3c, ASSERT…) ──"
    # Un chronicle rend 1 quand le banc invariant M3c casse — ce n'est PAS un plantage,
    # et le journal est complet : on NOMME la sim et la raison ici plutôt que de laisser
    # l'analyste chercher pourquoi le sweep sort en 4.
    { nbad=0
      for rc in "$OUT"/*.rc; do
          [ -f "$rc" ] || continue
          [ "$(cat "$rc")" = "0" ] && continue
          nbad=$((nbad+1))
          lg="${rc%.rc}.log"
          echo "  ⚠ $(basename "${rc%.rc}") → code $(cat "$rc")"
          grep -h "ÉCHEC" "$lg" 2>/dev/null | sed 's/^.*ÉCHEC/    ÉCHEC/' | head -3
      done
      [ "$nbad" -eq 0 ] && echo "  OK : les $(ls "$OUT"/*.rc 2>/dev/null | wc -l) sims rendent 0."
    }
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
