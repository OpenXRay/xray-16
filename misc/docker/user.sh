#!/usr/bin/env bash

if [[ $EUID == 0 ]]; then
  echo "This script must be run as non-root user inside docker group"
  exit 1
fi

docker exec -it xray-dev groupadd -g "$(id -g)" grp
docker exec -it xray-dev useradd -u "$(id -u)" -g "$(id -g)" -m "$USER"
docker exec -it xray-dev chown -R "$USER" /xray-16
