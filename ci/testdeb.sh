#!/bin/sh

dpkg -i ~/output/tlwbe_0.1_amd64.deb
apt-get -f install -y
