#!/bin/bash

for file in music/*.mp3; do
  echo "[INFO] Processing $file"
  
  artist=$(eyeD3 "$file" | grep "artist:" | head -1 | cut -d ':' -f2- | xargs)
  title=$(eyeD3 "$file" | grep "title:" | head -1 | cut -d ':' -f2- | xargs)

  if [[ -z "$artist" || -z "$title" ]]; then
    echo "     Missing tags. Skipping."
    continue
  fi

  newname="${artist// /_}-${title// /_}.mp3"
  newpath="music/$newname"

  if [[ "$file" != "$newpath" && ! -e "$newpath" ]]; then
    echo "    → Renaming to: $newname"
    mv "$file" "$newpath"
  else
    echo "    File already exists or name unchanged. Skipping."
  fi
done

echo "Renaming complete."
