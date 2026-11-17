#!/bin/bash
# Usage: WINEPREFIX=<throwaway prefix> bash check.sh path/to/menu.exe
# Runs menu.exe and prints, for each window, what the X server and the window
# manager see. Expected on a fixed Wine: caption managed, both menus
# override-redirect.
set -euo pipefail
here="$(dirname "$(readlink -f "$0")")"
exec 3< <(wine "$1" 2>/dev/null)
rows=""
for _ in 1 2 3; do IFS= read -r line <&3; rows+="$line"$'\n'; done
sleep 1
printf '%s' "$rows" | python3 "$here/check.py"
cat <&3 >/dev/null
