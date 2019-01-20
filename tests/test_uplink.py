import pytest
from tlwpy.tlwbe import Tlwbe, Result, Uplink
from tlwpy.gatewaysimulator import Gateway
from tlwpy.pktfwdbr import PacketForwarder
import asyncio


@pytest.mark.asyncio
async def test_uplink(mosquitto_process, tlwbe_process, dev, tlwbe_client: Tlwbe, gateway: Gateway,
                      pktfwdbr: PacketForwarder):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_eui = dev[0]
    dev_eui = dev[1]
    dev_key = dev[2]

    dev_addr, network_key, app_key = await gateway.join(app_eui, dev_eui, dev_key)
    assert 0 <= dev_addr <= 0xffffffff
    assert len(network_key) is 16
    assert len(app_key) is 16

    tlwbe_client.listen_for_uplinks(app_eui, dev_eui, 1)
    framecounter = 0
    payload = b'abc123'
    await gateway.send_uplink(2, dev_addr, framecounter, 1, network_key, app_key, payload=payload)
    framecounter += 1
    uplink: Uplink = await asyncio.wait_for(tlwbe_client.queue_uplinks.get(), 10)
    assert len(uplink.payload) == len(payload)
    assert uplink.payload == payload
