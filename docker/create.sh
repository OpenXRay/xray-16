#!/usr/bin/env bash

if [[ $EUID == 0 ]]; then
  echo "This script must be run as non-root user inside docker group"
  exit 1
fi

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
mount_path=$DIR/../
echo "Mounting from $mount_path"
sed "s/<mount_path>/${mount_path//\//\\/}/" "$DIR/docker-compose.yaml" >"$DIR/docker-compose.yml"

CONT_UID=$(id -u)
CONT_GID=$(id -g)
export CONT_UID
export CONT_GID

docker-compose -f "$DIR/docker-compose.yml" up -d --build --force-recreate
docker exec -it xray-image groupadd -g "$(id -g)" grp
docker exec -it xray-image useradd -u "$(id -u)" -g "$(id -g)" -m "$USER"
docker exec -it xray-image chown -R "$USER" /xray-16
