FROM debian:testing
RUN apt-get -qq update
RUN apt-get -qq install -y build-essential pkg-config libjson-glib-dev libsqlite3-dev libssl-dev libmosquitto-dev meson
ADD . /root