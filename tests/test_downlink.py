import pytest
from tlwpy.tlwbe import Tlwbe, Result
from tlwpy.gatewaysimulator import Gateway, Node
from tlwpy.pktfwdbr import PacketForwarder
from tlwpy.lorawan import JoinAccept
import asyncio


@pytest.mark.asyncio
async def test_downlink(mosquitto_process, tlwbe_process, dev, tlwbe_client: Tlwbe, gateway: Gateway,
                        pktfwdbr: PacketForwarder):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_eui = dev[0]
    dev_eui = dev[1]
    dev_key = dev[2]

    # create a node and join
    node = Node(gateway, app_eui, dev_eui, dev_key)
    await node.join()

    # schedule a downlink and then check it's scheduled
    result: Result = await tlwbe_client.send_downlink(app_eui, dev_eui, 1, b'123abc', False)
    assert result.code == 0
    result: Result = await tlwbe_client.list_downlinks(dev_eui=dev_eui)

    # send an uplink from the device
    await node.send_uplink(1, payload=b'abc123')

    await asyncio.wait_for(pktfwdbr.downlinks.get(), 10)
