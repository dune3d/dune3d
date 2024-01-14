#!/usr/bin/env bash
MISSING=$(ldd dist/dune3d/dune3d.exe | grep -vi windows | grep -vi dist/dune3d | grep -v "???")
if [ -z "$MISSING" ]
then
  echo "No missing DLLs"
else
  echo "Missing DLLs"
  echo "$MISSING"
  exit 1
fi
