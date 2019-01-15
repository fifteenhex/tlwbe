#!/bin/sh

set -e

set +e
dpkg -i ~/output/tlwbe_0.1_amd64.deb
set -e

apt-get -f install -y
pip3 install git+https://github.com/fifteenhex/tlwpy.git@master
git clean -fd
pytest-3 tests/test_control.py --log-level=debug
pytest-3 tests/test_downlink.py --log-level=debug
