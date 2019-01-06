import pytest


def pytest_addoption(parser):
    parser.addoption("--mosquitto_path", action="store")
    parser.addoption("--tlwbe_path", action="store")
    parser.addoption("--tlwbe_database_path", action="store")
    parser.addoption("--tlwbe_regionalparameters_path", action="store")


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
