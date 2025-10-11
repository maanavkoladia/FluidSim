#!/bin/bash
set -e

elfPath="build/FuildSim.exe"

make clean
bear -- make -j

echo "compilation succeeded"

#./"$elfPath"
