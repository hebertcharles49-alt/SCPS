#!/bin/bash
# GIGA SWEEP chronicle — 12 graines × 5 sims × 250 ans (config par défaut 6 empires/12 cités)
# + 2 graines « grand monde » (10 empires / 16 cités) × 3 sims × 250 ans.
# Un log par run ; TIMING en tête. Lancé depuis Git Bash (les DLL mingw résolvent).
cd /c/Users/Charl/Desktop/SCPS-main || exit 1
mkdir -p sweep/logs
SEEDS="3 5 7 9 11 19 42 77 99 123 145 777"
for s in $SEEDS; do
  echo "=== seed $s (5 sims x 250 ans) ==="
  t0=$(date +%s)
  ./chronicle.exe "$s" 5 250 > "sweep/logs/seed_${s}.log" 2>&1
  rc=$?
  t1=$(date +%s)
  echo "seed $s : rc=$rc en $((t1-t0)) s"
  echo "RC=$rc DUREE=$((t1-t0))s" >> "sweep/logs/seed_${s}.log"
done
for s in 9 42; do
  echo "=== seed $s GRAND (10 empires/16 cites, 3 sims x 250 ans) ==="
  t0=$(date +%s)
  ./chronicle.exe "$s" 3 250 10 16 > "sweep/logs/seed_${s}_grand.log" 2>&1
  rc=$?
  t1=$(date +%s)
  echo "seed $s grand : rc=$rc en $((t1-t0)) s"
  echo "RC=$rc DUREE=$((t1-t0))s" >> "sweep/logs/seed_${s}_grand.log"
done
echo "SWEEP TERMINE"
