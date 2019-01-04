import pytest
from subprocess import Popen
import time


@pytest.fixture
def get_mosquitto():
    return Popen(['mosquitto', '-p', '6666'])


@pytest.fixture
def get_tlwbe():
    return Popen(['tlwbe', '-h', 'localhost', '-p', '6666', '--region=as920_923'])


def test_addapp():
    mosq = get_mosquitto()
    tlwbe = get_tlwbe()

    time.sleep(10)

    # assert mosq.poll() is None
    # assert tlwbe.poll() is None

    # assert False
