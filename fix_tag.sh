#!/bin/bash

# Ensure eyeD3 is installed
if ! command -v eyeD3 &> /dev/null; then
  echo "eyeD3 not found. Please install it first."
  exit 1
fi

# Sample data pools
titles=(
  "Midnight_Drive" "Neon_Skies" "Retro_Pulse" "Sunset_Soul" "Echoes" "Starfields"
  "Dreamline" "Urban_Mirage" "Nocturnal_Flow" "Wavescape" "Moonlight_Drift" "Electric_Rain"
  "Golden_Hour" "Chill_Sequence" "Digital_Escape" "Ocean_Bloom" "Lucid_Flux"
  "Twilight_Run" "Deep_Current" "Silent_Voltage" "Cloud_Breaker" "Crystal_Maze"
  "Signal_Lost" "Gravity_Echo" "Solar_Bloom" "Bass_Orbit" "Hollow_Horizon"
  "Shimmer_Path" "Cyan_Dust" "Dusk_Reaction" "The_Last_Glow" "Binary_Solace"
)
artists=("DJ_Nova" "SynthFox" "Lunar_Kid" "EchoMind" "Waveform" "Chroma" "BassSmith" "NeonJack")
albums=("Neon_City" "Starlight_Express" "Future_Lounge" "Bassline_Theory" "Crystal_Grid" "Dream_Engine")
genres=(0 1 4 7 13 17 22 25 32 52)  # Valid ID3v1 genre IDs
years=($(seq 1990 2024))

counter=0

for file in music/*.mp3; do
  echo "[INFO] Cleaning & tagging: $file"

  # Remove old tags
  eyeD3 --remove-all "$file" > /dev/null 2>&1

  # Generate random metadata
  title=${titles[$RANDOM % ${#titles[@]}]}
  artist=${artists[$RANDOM % ${#artists[@]}]}
  album=${albums[$RANDOM % ${#albums[@]}]}
  genre=${genres[$RANDOM % ${#genres[@]}]}
  year=${years[$RANDOM % ${#years[@]}]}

  # Truncate for ID3v1 compatibility
  title="${title:0:30}"
  artist="${artist:0:30}"
  album="${album:0:30}"

  # Apply new tag (note: eyeD3 may not support --year or --genre directly in ID3v1)
  eyeD3 --to-v1.1 \
    --title "$title" \
    --artist "$artist" \
    --album "$album" \
    "$file" > /dev/null

  # Manually write the year (bytes 93–96 of last 128)
  printf "%-4s" "$year" | dd of="$file" bs=1 seek=$(( $(stat -c%s "$file") - 35 )) conv=notrunc status=none

  # Write the genre ID (last byte of file)
  printf "\\x$(printf '%02x' $genre)" | dd of="$file" bs=1 seek=$(( $(stat -c%s "$file") - 1 )) conv=notrunc status=none

  echo "    → Set title: $title | artist: $artist | album: $album | year: $year | genre ID: $genre"

  # Rename file to clean name
  clean_title=${title// /_}
  clean_artist=${artist// /_}
  newname="${clean_artist}-${clean_title}.mp3"
  newpath="music/$newname"

  if [[ "$file" != "$newpath" ]]; then
    mv "$file" "$newpath"
    echo "    → Renamed to: $newname"
  fi

  ((counter++))
done

echo "Tagging complete: $counter files updated."
