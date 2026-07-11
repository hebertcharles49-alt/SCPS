#!/usr/bin/env bash
# GIGA SWEEP 200 sims : 20 graines × 10 sims × 250 ans — l'arbitre (gel moteur S).
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
LOG=gigasweep_2026-07-11.log
: > "$LOG"
for seed in 3 7 9 11 19 42 77 99 108 111 123 145 209 310 411 555 777 888 1234 4242; do
  echo "=== GRAINE $seed ===" >> "$LOG"
  ./chronicle "$seed" 10 250 6 12 >> "$LOG" 2>&1
  echo "--- graine $seed EXIT=$? ---" >> "$LOG"
done
echo "GIGASWEEP TERMINÉ" >> "$LOG"
