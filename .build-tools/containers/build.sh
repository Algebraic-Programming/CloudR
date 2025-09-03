#!/usr/bin/env bash

if command -v arch &>/dev/null; then
   target_arch=$(arch)

   if [ $target_arch == "aarch64" ]; then
      target_arch="arm64"
   else
      target_arch="amd64"
   fi
else
   if [[ $# -ne 2 ]]; then
      echo "arch not installed. Please provice manually the target architecture. Usage: $0 <name> <arch>"
      exit 1
   else
      target_arch=${2}
   fi
fi

folder=${1}
echo "Building $folder for arch $target_arch"

docker_base_image=registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/${folder}

docker build -t "${docker_base_image}:latest-${target_arch}" ${folder} 