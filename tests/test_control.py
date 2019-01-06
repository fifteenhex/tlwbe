import pytest
from subprocess import Popen
import time
from tlwpy.tlwbe import Tlwbe, Result

MQTT_PORT = 6666


@pytest.fixture(scope="session")
def mosquitto_process(mosquitto_path):
    process = Popen([mosquitto_path, '-v', '-p', str(MQTT_PORT)])
    time.sleep(10)
    yield process
    process.terminate()
    process.wait()


@pytest.fixture(scope="session")
def tlwbe_process(tlwbe_path, tlwbe_database_path, tlwbe_regionalparameters_path):
    args = [tlwbe_path, '-h', 'localhost', '-p', str(MQTT_PORT), '--region=as920_923']
    if tlwbe_database_path is not None:
        args.append('--database_path=%s' % tlwbe_database_path)
    if tlwbe_regionalparameters_path is not None:
        args.append('--regionalparameters_path=%s' % tlwbe_regionalparameters_path)
    process = Popen(args)
    time.sleep(10)
    yield process
    process.terminate()
    process.wait()


@pytest.fixture()
async def tlwbe_client():
    tlwbe = Tlwbe('localhost', port=MQTT_PORT)
    return tlwbe


async def add_app(tlwbe_client):
    app_result: Result = await tlwbe_client.add_app('myapp')
    assert app_result.code == 0
    return app_result


@pytest.mark.asyncio
async def test_app(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_result = await add_app(tlwbe_client)
    eui = app_result.result['app']['eui']
    list_result: Result = await tlwbe_client.list_apps()
    assert eui in list_result.result['eui_list']
    delete_result: Result = await tlwbe_client.delete_app(eui)
    list_result = await  tlwbe_client.list_apps()
    assert eui not in list_result.result['eui_list']


@pytest.mark.asyncio
async def test_dev(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_result = await add_app(tlwbe_client)
    app_eui = app_result.result['app']['eui']
    dev_result: Result = await tlwbe_client.add_dev('mydev', app_eui)
    assert dev_result.code == 0
    dev_eui = dev_result.result['dev']['eui']
    list_result: Result = await tlwbe_client.list_devs()
    assert dev_eui in list_result.result['eui_list']
    delete_result: Result = await tlwbe_client.delete_dev(dev_eui)
    list_result = await  tlwbe_client.list_devs()
    assert dev_eui not in list_result.result['eui_list']
