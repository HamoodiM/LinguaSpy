#!/usr/bin/env bash
# Packages a Shipping Windows build into Saved/Packaged and adds the pilot launcher + credits.
# Run with the editor CLOSED (it builds the game target). Takes 20-60 min the first time (engine + shaders).
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
"/c/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun \
  -project="$ROOT/Personal.uproject" -noP4 -platform=Win64 -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -archive -archivedirectory="$ROOT/Saved/Packaged" -unattended -utf8output
cp "$ROOT/Tools/Packaging/Start LinguaSpy (Pilot).bat" "$ROOT/Saved/Packaged/Windows/"
cp "$ROOT/docs/Credits.md" "$ROOT/Saved/Packaged/Windows/CREDITS.md"
echo "Build: $ROOT/Saved/Packaged/Windows"
