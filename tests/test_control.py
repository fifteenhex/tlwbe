import pytest
from subprocess import Popen
import time
from tlwpy.tlwbe import Tlwbe

MQTT_PORT = 6666


@pytest.fixture
def mosquitto_process(mosquitto_path):
    process = Popen([mosquitto_path, '-p', str(MQTT_PORT)])
    yield process
    process.terminate()


@pytest.fixture
def tlwbe_process(tlwbe_path, tlwbe_database_path, tlwbe_regionalparameters_path):
    args = [tlwbe_path, '-h', 'localhost', '-p', str(MQTT_PORT), '--region=as920_923']
    if tlwbe_database_path is not None:
        args.append('--database_path=%s' % tlwbe_database_path)
    if tlwbe_regionalparameters_path is not None:
        args.append('--regionalparameters_path=%s' % tlwbe_regionalparameters_path)
    process = Popen(args)
    yield process
    process.terminate()


@pytest.fixture
async def tlwbe_client():
    return Tlwbe('localhost', port=MQTT_PORT)


@pytest.mark.asyncio
async def test_app_add(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    time.sleep(4)

    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    result = await tlwbe_client.add_app('myapp')
    print(result.payload)

    assert False


@pytest.mark.asyncio
async def test_app_list(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    time.sleep(4)

    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    result = await tlwbe_client.list_apps()

    assert False
