import pytest
from tlwpy.tlwbe import Tlwbe, Result
from subprocess import Popen
import time
from tlwpy.gatewaysimulator import Gateway
import random


def pytest_addoption(parser):
    parser.addoption("--mosquitto_path", action="store")
    parser.addoption("--tlwbe_path", action="store")
    parser.addoption("--tlwbe_database_path", action="store")
    parser.addoption("--tlwbe_regionalparameters_path", action="store")


@pytest.fixture(scope='session')
def mosquitto_port():
    return random.randint(6000, 8000)


@pytest.fixture(scope="session")
def mosquitto_path(request):
    path = request.config.getoption("--mosquitto_path")
    if path is None:
        path = "mosquitto"
    return path


@pytest.fixture(scope="session")
def tlwbe_path(request):
    path = request.config.getoption("--tlwbe_path")
    if path is None:
        path = "tlwbe"
    return path


@pytest.fixture(scope="session")
def tlwbe_database_path(request, tmpdir_factory):
    path = request.config.getoption("--tlwbe_database_path")
    if path is None:
        path = tmpdir_factory.mktemp('tlwbe').join('tlwbe.sqlite')
    return path


@pytest.fixture(scope="session")
def tlwbe_regionalparameters_path(request):
    return request.config.getoption("--tlwbe_regionalparameters_path")


@pytest.fixture(scope="session")
def mosquitto_process(mosquitto_path, mosquitto_port):
    process = Popen([mosquitto_path, '-v', '-p', str(mosquitto_port)])
    time.sleep(2)
    yield process
    process.terminate()
    process.wait()


@pytest.fixture(scope="session")
def tlwbe_process(mosquitto_port, tlwbe_path, tlwbe_database_path, tlwbe_regionalparameters_path):
    args = [tlwbe_path, '-h', 'localhost', '-p', str(mosquitto_port), '--region=as920_923']
    if tlwbe_database_path is not None:
        args.append('--database_path=%s' % tlwbe_database_path)
    if tlwbe_regionalparameters_path is not None:
        args.append('--regionalparameters_path=%s' % tlwbe_regionalparameters_path)
    process = Popen(args)
    time.sleep(2)
    yield process
    process.terminate()
    process.wait()


@pytest.fixture()
async def tlwbe_client(mosquitto_process, mosquitto_port, tlwbe_process):
    tlwbe = Tlwbe('localhost', port=mosquitto_port)
    await tlwbe.wait_for_connection()
    return tlwbe


@pytest.fixture()
async def app(tlwbe_client: Tlwbe):
    result: Result = await tlwbe_client.add_app('myapp')
    assert result.code == 0
    app_eui = result.result['app']['eui']
    yield app_eui
    await tlwbe_client.delete_app(app_eui)


@pytest.fixture()
async def dev(tlwbe_client: Tlwbe, app):
    result: Result = await tlwbe_client.add_dev('mydev', app)
    assert result.code == 0
    dev_eui = result.result['dev']['eui']
    dev_key = result.result['dev']['key']
    yield (app, dev_eui, dev_key)
    await tlwbe_client.delete_dev(dev_eui)


@pytest.fixture()
async def gateway(mosquitto_process, mosquitto_port, tlwbe_process):
    gateway = Gateway('localhost', port=mosquitto_port)
    await gateway.wait_for_connection()
    return gateway
