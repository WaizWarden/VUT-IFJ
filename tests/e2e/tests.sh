#!/bin/bash

# AI USAGE - CLAUDE
# Completely created by AI

cd "$(dirname "${BASH_SOURCE[0]}")" || exit 2

compiler="../../Output"
interp="./ic26int-linux-x86_64"
tmpDir="tmp"

test -x "$compiler" || { echo "Compiler not found: $compiler (run make)"; exit 1; }
test -x "$interp"   || { echo "
Interpreter not found or missing permissions: $interp
First download it from elearning and add it to /tests/e2e
Add execute permissions
"; exit 1; }
hasScala=0
command -v scala >/dev/null 2>&1 && hasScala=1
skip=0

pass=0; fail=0; failed=()

mkdir -p "$tmpDir"

run_dir() {
  local dir="$1"

  for src in "$dir"/*.scala; do
    [ -e "$src" ] || continue

    local base="${src%.scala}"
    local name="$dir/$(basename "$base")"
    local tmpName="${dir}_$(basename "$base")"

    local input="$base.in"
    test -f "$input" || input=/dev/null

    # Compile
    "$compiler" < "$src" > "$tmpDir/$tmpName.code" 2> "$tmpDir/$tmpName.cerr"
    local crc=$?

    # Expected compile error: file name.err contains the expected code
    if [ -f "$base.err" ]; then
      local expectedRc
      expectedRc="$(tr -d '[:space:]' < "$base.err")"
      if [ "$crc" = "$expectedRc" ]; then
        echo "PASS $name (compile error $crc)"
        pass=$((pass + 1))
      else
        echo "FAIL $name: compiler rc=$crc, expected $expectedRc"
        fail=$((fail + 1)); failed+=("$name")
      fi
      continue
    fi

    if [ $crc -ne 0 ]; then
      echo "FAIL $name: compiler rc=$crc"
      fail=$((fail + 1)); failed+=("$name")
      continue
    fi

    # Interpreter
    "$interp" "$tmpDir/$tmpName.code" < "$input" > "$tmpDir/$tmpName.actual" 2> "$tmpDir/$tmpName.ierr"
    local irc=$?
    if [ $irc -ne 0 ]; then
      echo "FAIL $name: interpreter rc=$irc ($(head -n1 "$tmpDir/$tmpName.ierr"))"
      fail=$((fail + 1)); failed+=("$name")
      continue
    fi

        # Compare
    local expected="$base.out"
    if [ ! -f "$expected" ]; then
      if [ $hasScala -eq 1 ]; then
        expected="$tmpDir/$tmpName.out"
        if ! scala run ifj26.scala "$src" < "$input" > "$expected" 2> "$tmpDir/$tmpName.scalaerr"; then
          echo "FAIL $name: Scala reference program failed"
          fail=$((fail + 1)); failed+=("$name")
          continue
        fi
      else
        echo "SKIP $name: no $base.out and Scala not installed"
        skip=$((skip + 1))
        continue
      fi
    fi
    if ! diff -q "$tmpDir/$tmpName.actual" "$expected" > /dev/null; then
      echo "FAIL $name: output differs"
      fail=$((fail + 1)); failed+=("$name")
      continue
    fi

    echo "PASS $name"
    pass=$((pass + 1))
  done
}

run_dir "basic"
run_dir "custom"
run_dir "extensions"


#rm -rf "$tmpDir"
echo "passed: $pass, failed: $fail"
if [ $fail -ne 0 ]; then
  printf '  - %s\n' "${failed[@]}"
  exit 1
fi