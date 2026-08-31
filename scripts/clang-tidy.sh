#!/usr/bin/env bash
# Runs clang-tidy over the project's own sources and fails if anything is reported.
#
# This wraps run-clang-tidy to paper over two portability problems, both caused by
# compile_commands.json being written for GCC and then replayed by clang:
#
#   * Distro Qt builds propagate GCC-only flags through their CMake config -- Arch's
#     qt6-base adds -mno-direct-extern-access. clang's driver rejects unknown arguments
#     outright, and there is no flag to downgrade that to a warning.
#   * CMake omits -std entirely when the compiler's default already satisfies the
#     requested standard. GCC 16 defaults to C++20, so nothing is emitted, and clang
#     (defaulting to C++17) then fails on every std::ranges / std::span / <=> use.
#
# Both are fixed by rewriting the compile commands into a scratch copy, leaving the real
# build tree untouched.
#
# Usage: scripts/clang-tidy.sh [build-dir] [extra run-clang-tidy args...]
#   scripts/clang-tidy.sh              # check, auto-detecting the build directory
#   scripts/clang-tidy.sh build -fix   # check and apply the suggested fixes
set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
readonly repo_root

if [[ -n ${1:-} && -d ${1:-} ]]; then
  build_dir=$1
  shift
else
  for candidate in "$repo_root/build/Release" "$repo_root/build"; do
    if [[ -f $candidate/compile_commands.json ]]; then
      build_dir=$candidate
      break
    fi
  done
fi

if [[ -z ${build_dir:-} || ! -f $build_dir/compile_commands.json ]]; then
  echo "clang-tidy.sh: no compile_commands.json found in ${build_dir:-build/Release or build}" >&2
  echo "Configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first." >&2
  exit 1
fi

scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

python3 - "$build_dir/compile_commands.json" "$scratch/compile_commands.json" <<'PY'
import json
import shlex
import sys

# GCC-only flags that clang's driver rejects as unknown arguments.
UNSUPPORTED = {"-mno-direct-extern-access"}
STANDARD = "-std=gnu++20"

source, destination = sys.argv[1], sys.argv[2]
with open(source) as handle:
    entries = json.load(handle)

for entry in entries:
    # Either key is valid in the JSON Compilation Database format; CMake writes "command".
    key = "command" if "command" in entry else "arguments"
    words = shlex.split(entry[key]) if key == "command" else list(entry[key])
    words = [word for word in words if word not in UNSUPPORTED]
    if not any(word.startswith("-std=") for word in words):
        words.append(STANDARD)
    entry[key] = shlex.join(words) if key == "command" else words

with open(destination, "w") as handle:
    json.dump(entries, handle)
PY

# Distro packages install the driver unsuffixed; the clang-tidy wheel on PyPI, which CI
# uses to pin the version, ships it as run-clang-tidy.py.
for candidate in run-clang-tidy run-clang-tidy.py; do
  if command -v "$candidate" > /dev/null; then
    runner=$candidate
    break
  fi
done

if [[ -z ${runner:-} ]]; then
  echo "clang-tidy.sh: neither run-clang-tidy nor run-clang-tidy.py is on PATH" >&2
  exit 1
fi

# The header filter is an llvm::Regex (no lookahead); vendored and generated headers are
# dropped by ExcludeHeaderFilterRegex in .clang-tidy instead. The trailing positional
# argument is a Python regex, so it can exclude the vendored directory inline.
exec "$runner" \
  -p "$scratch" \
  -quiet \
  -warnings-as-errors='*' \
  -header-filter="^$repo_root/(src|test|benchmarks)/" \
  "$@" \
  "^$repo_root/(src|test|benchmarks)/(?!kdsingleapplication/)"
