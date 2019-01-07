import pytest
from tlwpy.tlwbe import Tlwbe, Result


@pytest.mark.asyncio
async def test_downlink(mosquitto_process, tlwbe_process, dev, tlwbe_client: Tlwbe):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    result: Result = await tlwbe_client.send_downlink(dev[0], dev[1], 1, b'123abc', False)
    assert result.code == 0
