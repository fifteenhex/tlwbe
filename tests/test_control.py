import pytest
from subprocess import Popen
import time
from tlwpy.tlwbe import Tlwbe

MQTT_PORT = 6666


@pytest.fixture
def mosquitto_process():
    return Popen(['mosquitto', '-p', str(MQTT_PORT)])


@pytest.fixture
def tlwbe_process():
    return Popen(['tlwbe', '-h', 'localhost', '-p', '6666', '--region=as920_923'])


@pytest.fixture
async def tlwbe_client():
    return Tlwbe('localhost', port=MQTT_PORT)


@pytest.mark.asyncio
async def test_addapp(mosquitto_process, tlwbe_process, tlwbe_client : Tlwbe):
    time.sleep(10)

    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None



    assert False
