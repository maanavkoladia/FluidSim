#!/bin/bash

# Usage: ./convert_frames.sh <frame_dir> <output_file> [width] [height] [framerate]
# Example: ./convert_frames.sh frames output.mp4 1920 1080 60

frameDirPath=$1
outFileName=$2
width=${3:-480}
height=${4:-480}
framerate=${5:-100}

# Check if required arguments are provided
if [ -z "$frameDirPath" ] || [ -z "$outFileName" ]; then
    echo "Usage: $0 <frame_dir> <output_file> [width] [height] [framerate]"
    echo "Example: $0 frames output.mp4 1920 1080 60"
    exit 1
fi

# Check if frame directory exists
if [ ! -d "$frameDirPath" ]; then
    echo "Error: Frame directory '$frameDirPath' does not exist"
    exit 1
fi

# Count frames
frame_count=$(ls -1 "$frameDirPath"/frame_*.rgba 2>/dev/null | wc -l)
if [ "$frame_count" -eq 0 ]; then
    echo "Error: No .rgba frames found in '$frameDirPath'"
    exit 1
fi

echo "Found $frame_count frames in $frameDirPath"
echo "Converting to $outFileName at ${width}x${height} @ ${framerate}fps..."

# Run ffmpeg with the provided parameters
ffmpeg -y \
    -f rawvideo \
    -pixel_format rgba \
    -video_size ${width}x${height} \
    -framerate ${framerate} \
    -i "$frameDirPath/frame_%05d.rgba" \
    -c:v libx264 \
    -preset fast \
    -crf 18 \
    -pix_fmt yuv420p \
    "$outFileName"

# Check if ffmpeg succeeded
if [ $? -eq 0 ]; then
    echo "Success! Video created: $outFileName"
    ls -lh "$outFileName"
else
    echo "Error: ffmpeg conversion failed"
    exit 1
fi
