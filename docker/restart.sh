#!/usr/bin/env bash

if [[ $EUID == 0 ]]; then
  echo "This script must be run as non-root user inside docker group"
  exit 1
fi

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
export XRAY_MOUNT_PATH=$DIR/../

docker-compose -f "$DIR/docker-compose.yml" up -d
