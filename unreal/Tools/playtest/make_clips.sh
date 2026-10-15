#!/bin/zsh
# Cut the frames recorded by record_clips.py into mp4 (and a small gif) per clip: make_clips.sh <outdir> [name ...]
set -e
OUT=${1:-/tmp/clips}; shift || true
SHOTS="$(cd "$(dirname "$0")/../../Saved/Screenshots/MacEditor" && pwd)"
mkdir -p "$OUT"
for name in ${@:-walk ride climb channel}; do
  n=$(ls "$SHOTS" | grep -c "^rec_${name}_" || true)
  [ "$n" -gt 0 ] || { echo "no frames for $name"; continue; }
  # frames with UI are the whole editor window (1920x1050 here): crop to the PIE viewport incl. its HUD;
  # frames without UI are the letterboxed game image: crop the black bars off
  if [ "$name" = "walk" ]; then CROP="scale=1920:1050,crop=1453:515:0:138,"; else CROP="crop=930:522:260:0,"; fi
  ffmpeg -y -loglevel error -framerate 6 -pattern_type glob -i "$SHOTS/rec_${name}_*.png" \
    -vf "${CROP}scale=1200:-2:flags=lanczos,format=yuv420p" -c:v libx264 -crf 24 -r 12 -movflags +faststart "$OUT/$name.mp4"
  ffmpeg -y -loglevel error -framerate 6 -pattern_type glob -i "$SHOTS/rec_${name}_*.png" \
    -vf "${CROP}fps=5,scale=640:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=bayer:bayer_scale=4" "$OUT/$name.gif"
  ls -la "$OUT/$name.mp4" "$OUT/$name.gif" | awk '{print $5, $9}'
done
