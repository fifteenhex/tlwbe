#!/bin/sh

docker build -f ci/Dockerfile.build -t tlwbe_buildenv .
docker run -v tlwbe_build_output:/root/output tlwbe_buildenv sh -c "cd /root/tlwbe && ./ci/builddeb.sh"
