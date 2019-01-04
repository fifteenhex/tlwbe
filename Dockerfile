FROM debian:testing
RUN apt-get -qq update
RUN apt-get -qq dist-upgrade
RUN apt-get -qq install -y build-essential \
                           pkg-config \
                           mosquitto \
                           meson \
                           git-buildpackage \
                           python3-pip \
                           python3-pytest
RUN pip3 install git+https://github.com/fifteenhex/tlwpy.git@master
ADD . /root/tlwbe
