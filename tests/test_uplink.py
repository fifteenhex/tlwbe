import pytest
from tlwpy.tlwbe import Tlwbe, Result, Uplink
from tlwpy.gatewaysimulator import Gateway, Node
from tlwpy.pktfwdbr import PacketForwarder
from tlwpy.lorawan import Downlink
import asyncio


async def send_uplink(tlwbe_client, pktfwdbr, node, confirmed=False):
    payload = b'abc123'
    await node.send_uplink(port=1, confirmed=confirmed, payload=payload)
    uplink: Uplink = await asyncio.wait_for(tlwbe_client.queue_uplinks.get(), 10)
    assert len(uplink.payload) == len(payload)
    assert uplink.payload == payload

    if confirmed:
        downlink: Downlink = await asyncio.wait_for(pktfwdbr.downlinks.get(), 10)
        assert downlink.ack


@pytest.mark.asyncio
async def test_uplink(mosquitto_process, tlwbe_process, dev, tlwbe_client: Tlwbe, gateway: Gateway,
                      pktfwdbr: PacketForwarder):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_eui = dev[0]
    dev_eui = dev[1]
    dev_key = dev[2]

    # create a node and join
    node = Node(gateway, app_eui, dev_eui, dev_key)
    await node.join()

    # start listening for uplinks
    tlwbe_client.listen_for_uplinks(app_eui, dev_eui, 1)

    # send an unconfirmed uplink
    await send_uplink(tlwbe_client, pktfwdbr, node)

    # send a confirmed uplink
    await send_uplink(tlwbe_client, pktfwdbr, node, confirmed=True)
