#!/usr/bin/env bash

set -euxo pipefail

# debug
scons -Q \
      --enable-werror \
      --enable-debug \
      --enable-tests \
      --enable-examples \
      --compiler=clang-13 \
      --build-3rdparty=all \
      test

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read t
    do
        python3 scripts/scons_helpers/run-with-timeout.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                $t
    done

# release
scons -Q \
      --enable-werror \
      --enable-tests \
      --enable-examples \
      --compiler=clang-13 \
      --build-3rdparty=all \
      test

find bin/x86_64-pc-linux-gnu -name 'roc-test-*' |\
    while read t
    do
        python3 scripts/scons_helpers/run-with-timeout.py 300 \
            valgrind \
                --max-stackframe=10475520 \
                --error-exitcode=1 --exit-on-first-error=yes \
                $t
    done
