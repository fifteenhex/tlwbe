#!/bin/sh

set -e

set +e
dpkg -i ~/output/tlwbe_0.1_amd64.deb
set -e

apt-get -f install -y
pytest-3

