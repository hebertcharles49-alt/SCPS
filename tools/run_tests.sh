#!/usr/bin/env bash
# tools/run_tests.sh — l'instrument de non-régression (Arc K3.3).
# Bâtit et lance les bancs non-SDL auto-vérifiants, compte les BILAN, sort un
# récap vert/rouge et un code retour ≠ 0 si un banc échoue OU dépasse son temps.
#
#   run_tests.sh [full|smoke]      (défaut : full — tous les bancs)
#     full  : le gardien COMPLET (tous les bancs, audit_eco & lang_demo inclus).
#     smoke : un sous-ensemble RAPIDE (colonne vertébrale + bornes éco/langue)
#             pour un feedback en quelques secondes dans la boucle serrée du dev.
#
#   BANC_TIMEOUT=<s> : plafond de temps PAR banc (défaut 120). Un banc qui PEND
#                      est compté ROUGE (timeout) au lieu de bloquer la suite —
#                      c'est le garde-fou contre un banc qui part en boucle.
set -u
cd "$(dirname "$0")/.."

mode="${1:-full}"

# Le gardien COMPLET : tout banc auto-vérifiant non-SDL. audit_eco & lang_demo
# sont désormais INCLUS — les bornes de l'arc « une économie » et le lexique du
# readout ne doivent plus passer sous le radar (le rouge de l'audit éco est
# longtemps resté invisible faute d'être ici).
BENCHES_FULL=(
  core_demo monde_reel readout_demo species_demo tech_demo faith_demo
  intertrade_demo routes_demo save_io_demo statecraft_demo pop_demo army_demo
  demography_demo demography_integ_demo revolt_demo social_demo agency_demo
  campaign_demo factions_demo econ_tax_demo econ_culture_demo econ_arcane_demo
  econ_production_demo labor_demo missions_demo ai_demo diplo_demo warhost_demo
  events_demo structural_demo forks_demo prosperity_demo credit_demo cap_demo
  endgame_demo audit_eco lang_demo scps_api_demo
)

# Le sous-ensemble RAPIDE : la colonne vertébrale (worldgen/readout/labor/éco/IA),
# les deux NOUVELLES bornes (audit éco + langue) et l'endgame. Assez pour attraper
# une régression franche en quelques secondes ; le gardien complet reste `make test`.
BENCHES_SMOKE=(
  core_demo readout_demo labor_demo econ_production_demo ai_demo
  endgame_demo audit_eco lang_demo
)

case "$mode" in
  smoke) BENCHES=("${BENCHES_SMOKE[@]}"); echo "── mode SMOKE (sous-ensemble rapide) ──" ;;
  full)  BENCHES=("${BENCHES_FULL[@]}") ;;
  *) echo "usage: run_tests.sh [full|smoke]" >&2; exit 2 ;;
esac

TIMEOUT="${BANC_TIMEOUT:-120}"
have_timeout=0; command -v timeout >/dev/null 2>&1 && have_timeout=1

run_banc(){  # $1 = binaire ; borne le temps si `timeout` est dispo (124 = dépassé)
  if [ "$have_timeout" -eq 1 ]; then timeout "$TIMEOUT" ./"$1" 2>&1
  else ./"$1" 2>&1; fi
}

green=0 red=0 buildfail=0 timedout=0
red_list="" build_list="" timeout_list=""
printf "%-26s %s\n" "BANC" "RÉSULTAT"
printf '%.0s-' {1..50}; echo
for b in "${BENCHES[@]}"; do
  if ! make "$b" >/tmp/k3_build.log 2>&1; then
    printf "%-26s \033[31mBUILD ÉCHEC\033[0m\n" "$b"
    buildfail=$((buildfail+1)); build_list="$build_list $b"; continue
  fi
  out=$(run_banc "$b"); rc=$?
  if [ "$rc" -eq 124 ] && [ "$have_timeout" -eq 1 ]; then
    printf "%-26s \033[31mTIMEOUT (>%ss)\033[0m\n" "$b" "$TIMEOUT"
    red=$((red+1)); timedout=$((timedout+1)); timeout_list="$timeout_list $b"; continue
  fi
  # « X réussis, Y échoués » (banc auto-vérifiant)
  line=$(echo "$out" | grep -oE "[0-9]+ réussis, [0-9]+ échoués" | tail -1)
  if [ -n "$line" ]; then
    pass=$(echo "$line" | grep -oE "^[0-9]+")
    fail=$(echo "$line" | grep -oE "[0-9]+ échoués" | grep -oE "^[0-9]+")
    if [ "$fail" -eq 0 ]; then
      printf "%-26s \033[32m%s/%s ✓\033[0m\n" "$b" "$pass" "$pass"
      green=$((green+1))
    else
      printf "%-26s \033[31m%s/%s ✗\033[0m\n" "$b" "$pass" "$((pass+fail))"
      red=$((red+1)); red_list="$red_list $b"
    fi
  else
    # pas de format BILAN → on juge par le code retour de ./$b
    if [ "$rc" -eq 0 ]; then printf "%-26s \033[32mOK\033[0m\n" "$b"; green=$((green+1));
    else printf "%-26s \033[31mrc≠0\033[0m\n" "$b"; red=$((red+1)); red_list="$red_list $b"; fi
  fi
done
printf '%.0s-' {1..50}; echo
echo "VERTS : $green · ROUGES : $red · BUILD ÉCHEC : $buildfail   (sur ${#BENCHES[@]} bancs, mode $mode)"
[ -n "$red_list" ]     && echo "  rouges   :$red_list"
[ -n "$timeout_list" ] && echo "  timeouts :$timeout_list"
[ -n "$build_list" ]   && echo "  build    :$build_list"
[ $((red+buildfail)) -eq 0 ]
