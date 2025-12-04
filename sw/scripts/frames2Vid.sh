#!/bin/bash

frameDirPath=$1
outFileName=$2

ffmpeg -framerate 60 -i "${frameDirPath}/frame_%05d.png" -c:v libx264 -pix_fmt yuv420p "${outFileName}"
