import pytest
from tlwpy.tlwbe import Tlwbe, Result
from tlwpy.gatewaysimulator import Gateway
from tlwpy.pktfwdbr import PacketForwarder


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
    await gateway.send_uplink(2, dev_addr, 0, 0, network_key, app_key)
