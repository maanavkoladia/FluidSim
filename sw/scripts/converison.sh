#!/bin/bash

# Usage: ./frames2Vid.sh <frame_dir> [width height framerate]

DIR=$1
FILEOUT=${2:-output.mp4}
WIDTH=${3:-480}
HEIGHT=${4:-480}
FPS=${5:-100}   # optional framerate

if [ -z "$DIR" ]; then
    echo "Usage: $0 <frame_dir> [width height framerate]"
    exit 1
fi

outdir="${DIR}_png"
mkdir -p "$outdir"

echo "Converting RGBA → PNG: $DIR → $outdir"

for f in "$DIR"/frame_*.rgba; do
    base=$(basename "$f" .rgba)
    ffmpeg -v error \
        -f rawvideo \
        -pixel_format rgba \
        -video_size ${WIDTH}x${HEIGHT} \
        -i "$f" \
        -frames:v 1 \
        "$outdir/${base}.png"
done

echo "PNG conversion done."

# --- Convert PNGs to MP4 ---
OUT="${FILEOUT}.mp4"
echo "Converting PNGs → MP4: $OUT at ${WIDTH}x${HEIGHT} @ ${FPS}fps"

ffmpeg -y \
    -framerate $FPS \
    -i "${outdir}/frame_%05d.png" \
    -c:v libx264 -preset fast -crf 18 \
    -pix_fmt yuv420p \
    "$OUT"

echo "Done! Video created: $OUT"
ls -lh "$OUT"


