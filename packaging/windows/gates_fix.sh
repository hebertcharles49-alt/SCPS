#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp TMPDIR=/tmp PROCESSOR_ARCHITECTURE=AMD64
cd /c/Users/Charl/Desktop/SCPS-main
make scps_api_demo 2>&1 | grep -icE " error"
./scps_api_demo 2>&1 | grep -E "intrigue :|guerres |✗|BILAN" | head -10
