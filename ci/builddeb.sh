#!/bin/sh

mk-build-deps -i -t"apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends -y"
git checkout -b tmp
gbp buildpackage --git-ignore-new --no-sign
mv -v ../tlwbe_* ../tlwbe-* ~/output/
