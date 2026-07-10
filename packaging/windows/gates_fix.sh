#!/usr/bin/env bash
export TMP=/tmp TEMP=/tmp PATH="/d/MSYS2/mingw64/bin:$PATH"
cd /c/Users/Charl/Desktop/SCPS-main
gcc -fsyntax-only -std=c99 -Wall -Wextra -Ithird_party scps/scps_api.c scps/scps_decrees.c && echo SYNTAX_OK
