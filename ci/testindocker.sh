#!/bin/sh

docker build -f ci/Dockerfile.test -t tlwbe_testenv .
docker run -v tlwbe_build_output:/root/output tlwbe_testenv sh -c "cd /root/tlwbe && ./ci/testdeb.sh"
