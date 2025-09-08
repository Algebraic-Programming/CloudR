#!/bin/env bash

docker pull registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/buildenv:latest-amd64 
docker pull registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/buildenv:latest-arm64

docker buildx imagetools create --tag registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/buildenv:latest registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/buildenv:latest-amd64 registry.gitlab.huaweirc.ch/zrc-von-neumann-lab/runtime-system-innovations/cloudr/buildenv:latest-arm64