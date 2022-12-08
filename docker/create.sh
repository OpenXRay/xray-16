#!/usr/bin/env bash

if [[ $EUID == 0 ]]; then
  echo "This script must be run as non-root user inside docker group"
  exit 1
fi

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
export XRAY_MOUNT_PATH=$DIR/../
echo "Mounting from $XRAY_MOUNT_PATH"

CONT_UID=$(id -u)
CONT_GID=$(id -g)
export CONT_UID
export CONT_GID

docker-compose -f "$DIR/docker-compose.yml" up -d --build --force-recreate

$DIR/user.sh
