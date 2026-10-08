#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PEGTL_VERSION="3.2.7"
PEGTL_DIR="$ROOT/lib/PEGTL"

if [ -f "$PEGTL_DIR/include/tao/pegtl.hpp" ]; then
  exit 0
fi

mkdir -p "$ROOT/lib"
git clone -q --depth 1 --branch "$PEGTL_VERSION" https://github.com/taocpp/PEGTL.git "$PEGTL_DIR"
echo "PEGTL $PEGTL_VERSION installed in lib/PEGTL"
