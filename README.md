# TLWBE - tiny LoRaWAN back-end

[![Build Status](https://travis-ci.com/fifteenhex/tlwbe.svg?branch=master)](https://travis-ci.com/fifteenhex/tlwbe)

A very rough implementation of a backend for a LoraWAN network.
Intentially doesn't try to implement everything in the LoraWAN spec.
Should be used together with https://github.com/fifteenhex/pktfwdbr.

## Interface documentation

All interaction with tlwbe is done via mqtt using the *interfaces*
listed below. There is a basic *client* with a relatively userfriendly
interface in the client directory.

[Control interface](CONTROL.md)

[Join interface](JOIN.md)

[Uplink interface](UPLINK.md)

[Downlink interface](DOWNLINK.md)