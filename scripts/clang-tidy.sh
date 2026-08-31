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

# A leading non-option argument is the build directory. Tested for shape rather than for
# existence so that a mistyped or unpacked-too-late path fails here, instead of falling
# through to auto-detection and leaking the path into run-clang-tidy's file filters.
if [[ -n ${1:-} && ${1:-} != -* ]]; then
  build_dir=$1
  shift
  if [[ ! -d $build_dir ]]; then
    echo "clang-tidy.sh: build directory '$build_dir' does not exist" >&2
    exit 1
  fi
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

# Resolve the analyzer first, then look for its driver next to it. Order matters: Debian and
# Ubuntu patch their unsuffixed /usr/bin/run-clang-tidy to invoke the *versioned*
# clang-tidy-NN, so preferring a driver found on PATH silently bypasses whichever clang-tidy
# is pinned and picks up the distro's instead. Override with CLANG_TIDY=/path/to/clang-tidy.
tidy_binary=${CLANG_TIDY:-$(command -v clang-tidy || true)}
if [[ -z $tidy_binary ]]; then
  echo "clang-tidy.sh: clang-tidy is not on PATH (set CLANG_TIDY to override)" >&2
  exit 1
fi

# ExcludeHeaderFilterRegex in .clang-tidy needs 19 or newer; older releases reject the whole
# config file with an unhelpful "unknown key" error, so say so plainly instead.
tidy_version=$("$tidy_binary" --version | grep -oP '(?<=LLVM version )\d+' | head -1)
readonly minimum_version=19
if [[ -z $tidy_version ]] || ((tidy_version < minimum_version)); then
  echo "clang-tidy.sh: $tidy_binary reports version '${tidy_version:-unknown}';" \
    "$minimum_version or newer is required" >&2
  exit 1
fi

# The PyPI wheel ships the driver as run-clang-tidy.py, distro packages as run-clang-tidy.
tidy_dir=$(dirname "$tidy_binary")
for candidate in "$tidy_dir/run-clang-tidy" "$tidy_dir/run-clang-tidy.py" run-clang-tidy run-clang-tidy.py; do
  if command -v "$candidate" > /dev/null; then
    runner=$candidate
    break
  fi
done

if [[ -z ${runner:-} ]]; then
  echo "clang-tidy.sh: no run-clang-tidy driver found next to $tidy_binary or on PATH" >&2
  exit 1
fi

echo "clang-tidy.sh: using $tidy_binary (LLVM $tidy_version) via $runner"

# The header filter is an llvm::Regex (no lookahead); vendored and generated headers are
# dropped by ExcludeHeaderFilterRegex in .clang-tidy instead. The trailing positional
# argument is a Python regex, so it can exclude the vendored directory inline.
# -clang-tidy-binary is passed explicitly for the same reason the lookup above is ordered:
# the driver would otherwise choose the analyzer itself.
exec "$runner" \
  -p "$scratch" \
  -quiet \
  -clang-tidy-binary "$tidy_binary" \
  -warnings-as-errors='*' \
  -header-filter="^$repo_root/(src|test|benchmarks)/" \
  "$@" \
  "^$repo_root/(src|test|benchmarks)/(?!kdsingleapplication/)"
