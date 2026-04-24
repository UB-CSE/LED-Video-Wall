#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "This script must be run as root (use sudo)" >&2
  exit 1
fi

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <file>" >&2
  exit 1
fi

file=$1

if [ ! -e "$file" ]; then
  echo "Error: file does not exist: $file" >&2
  exit 1
fi

chown root "$file"
chmod 4755 "$file"

echo "Set owner root and mode 4755 on: $file"

