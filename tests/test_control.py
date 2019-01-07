import pytest
from tlwpy.tlwbe import Tlwbe, Result


async def add_app(tlwbe_client, app_name='myapp'):
    app_result: Result = await tlwbe_client.add_app(app_name)
    assert app_result.code == 0
    return app_result


@pytest.mark.asyncio
async def test_app(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    app_name = 'myapp'

    # add a new app and grab the eui
    app_result = await add_app(tlwbe_client, app_name)
    app_eui = app_result.result['app']['eui']

    # list the apps and check our app is in there
    list_result: Result = await tlwbe_client.list_apps()
    assert list_result.code == 0
    assert app_eui in list_result.result['eui_list']

    # try to get the app by it's eui and then it's name
    get_app_by_eui_result: Result = await tlwbe_client.get_app_by_eui(app_eui)
    get_app_by_name_result: Result = await tlwbe_client.get_app_by_name(app_name)

    # delete the app, get the list of apps and check its gone
    delete_result: Result = await tlwbe_client.delete_app(app_eui)
    list_result = await  tlwbe_client.list_apps()
    assert app_eui not in list_result.result['eui_list']


@pytest.mark.asyncio
async def test_dev(mosquitto_process, tlwbe_process, tlwbe_client: Tlwbe):
    assert mosquitto_process.poll() is None
    assert tlwbe_process.poll() is None

    dev_name = 'mydev'

    # add an app and grab the new eui
    app_result = await add_app(tlwbe_client)
    app_eui = app_result.result['app']['eui']

    # add a new dev to the app and grab the eui
    dev_result: Result = await tlwbe_client.add_dev(dev_name, app_eui)
    assert dev_result.code == 0
    dev_eui = dev_result.result['dev']['eui']

    # get a list of devs and check our eui is in there
    list_result: Result = await tlwbe_client.list_devs()
    assert list_result.code == 0
    assert dev_eui in list_result.result['eui_list']

    # try to get our dev by it's eui and then it's name
    get_dev_eui_result: Result = await tlwbe_client.get_dev_by_eui(dev_eui)
    assert get_dev_eui_result.code == 0
    get_dev_name_result: Result = await tlwbe_client.get_dev_by_name(dev_name)
    assert get_dev_name_result.code == 0

    # now delete the dev, get the list again and check it's not in there
    delete_result: Result = await tlwbe_client.delete_dev(dev_eui)
    assert delete_result.code == 0
    list_result = await tlwbe_client.list_devs()
    assert list_result.code == 0
    assert dev_eui not in list_result.result['eui_list']
