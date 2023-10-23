#!/usr/bin/env bash
set -e
bash ./scripts/run_clangformat.sh
if [[ -n $(git diff --name-status) ]]; then
  echo "Run clang-format before submitting..."
  echo ""
  echo "Files not conforming to style:"
  echo ""
  git --no-pager diff --name-status
  echo ""
  echo "Git diff:"
  git --no-pager diff
  exit -1
fi
echo "Style checks passed!"
