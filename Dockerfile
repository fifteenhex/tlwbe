FROM debian:testing
RUN apt-get -qq update
RUN apt-get -qq install -y build-essential \
                           pkg-config \
                           libjson-glib-dev \
                           libsqlite3-dev \
                           libssl-dev \
                           libmosquitto-dev \
                           meson \
                           python3-pycparser \
                           git-buildpackage \
                           python3-pip
RUN pip3 install git+https://github.com/fifteenhex/tlwpy.git@master
ADD . /root
