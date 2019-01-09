import pytest
from tlwpy.tlwbe import Tlwbe, Result
from tlwpy.gatewaysimulator import Gateway


@pytest.mark.asyncio
async def test_downlink(mosquitto_process, tlwbe_process, dev, tlwbe_client: Tlwbe, gateway: Gateway):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_eui = dev[0]
    dev_eui = dev[1]
    dev_key = dev[2]

    await gateway.join(app_eui, dev_eui, dev_key)

    result: Result = await tlwbe_client.send_downlink(app_eui, dev_eui, 1, b'123abc', False)
    assert result.code == 0
