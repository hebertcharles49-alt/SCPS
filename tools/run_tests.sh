#!/usr/bin/env bash
# tools/run_tests.sh — l'instrument de non-régression (Arc K3.3).
# Bâtit et lance tous les bancs non-SDL auto-vérifiants, compte les BILAN,
# sort un récap vert/rouge et un code retour ≠ 0 si un banc échoue.
set -u
cd "$(dirname "$0")/.."

BENCHES=(
  core_demo monde_reel readout_demo species_demo tech_demo faith_demo
  intertrade_demo routes_demo save_io_demo statecraft_demo pop_demo army_demo
  demography_demo demography_integ_demo revolt_demo social_demo agency_demo
  campaign_demo factions_demo econ_tax_demo econ_culture_demo econ_arcane_demo
  econ_production_demo labor_demo missions_demo ai_demo diplo_demo warhost_demo
  events_demo structural_demo forks_demo prosperity_demo
)

green=0 red=0 buildfail=0
red_list="" build_list=""
printf "%-26s %s\n" "BANC" "RÉSULTAT"
printf '%.0s-' {1..50}; echo
for b in "${BENCHES[@]}"; do
  if ! make "$b" >/tmp/k3_build.log 2>&1; then
    printf "%-26s \033[31mBUILD ÉCHEC\033[0m\n" "$b"
    buildfail=$((buildfail+1)); build_list="$build_list $b"; continue
  fi
  out=$(./"$b" 2>&1); rc=$?
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
echo "VERTS : $green · ROUGES : $red · BUILD ÉCHEC : $buildfail   (sur ${#BENCHES[@]} bancs)"
[ -n "$red_list" ]  && echo "  rouges :$red_list"
[ -n "$build_list" ] && echo "  build  :$build_list"
[ $((red+buildfail)) -eq 0 ]
