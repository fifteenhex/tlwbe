FROM debian:testing
RUN apt-get -qq update
RUN apt-get -qq install -y build-essential \
                           pkg-config \
                           libjson-glib-dev \
                           libsqlite3-dev \
                           libssl-dev \
                           mosquitto \
                           libmosquitto-dev \
                           meson \
                           git-buildpackage \
                           python3-pip \
                           python3-pytest \
                           python3-pycparser
RUN pip3 install git+https://github.com/fifteenhex/tlwpy.git@master
ADD . /root
