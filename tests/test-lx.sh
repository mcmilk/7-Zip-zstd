#!/bin/bash

# automatic setting via script is not working with linux
export Z7_REGR_TEST_DIR="/home/runner/work/7-Zip-zstd/7-Zip-zstd/tests/regr-arc"
export Z7_PATH="/home/runner/work/7-Zip-zstd/7-Zip-zstd/CPP/build/7z"
export TEMP="/tmp"

tclsh 7z-test.tcl -verbose tpsem
