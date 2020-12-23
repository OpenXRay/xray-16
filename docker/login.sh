#!/usr/bin/env bash

if [[ $EUID == 0 ]]; then
  echo "This script must be run as non-root user inside docker group"
  exit 1
fi

if [[ -z $xray_docker_start_directory ]]; then
  xray_docker_start_directory=/xray-16/
fi

docker exec -it --user "$(id -u)":"$(id -g)" xray-dev /bin/bash -c \
  "if [[ -d $xray_docker_start_directory ]]; then
    cd $xray_docker_start_directory
  fi
  /bin/bash"
