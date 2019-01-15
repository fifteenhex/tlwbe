#!/usr/bin/env python3

import argparse
from tlwpy.tlwbe import Tlwbe, RESULT_OK, Uplink
import asyncio
from prompt_toolkit import PromptSession, print_formatted_text, HTML
from prompt_toolkit.eventloop import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.document import Document
import prompt_toolkit.lexers
import prompt_toolkit.completion
import prompt_toolkit.contrib.regular_languages.compiler
import re
import string

use_asyncio_event_loop()


class Lexer(prompt_toolkit.lexers.SimpleLexer):
    pass


class Completer(prompt_toolkit.completion.Completer):
    def get_completions(self, document: Document, complete_event):

        word_start, word_end = document.find_boundaries_of_current_word()
        # print('%d %d' % (word_start, word_end))

        if word_start == 0 and word_end != 0:
            for command in misc:
                yield prompt_toolkit.completion.Completion(command)
            for command in object_types:
                yield prompt_toolkit.completion.Completion(command)


def cmd_unknown():
    print('I have no idea, sorry.')


async def cmd_help(tlwbe: Tlwbe, parameters: dict):
    print_formatted_text(HTML('<b>Usage:</b>'))
    print_formatted_text(HTML('OBJECT COMMAND [PARAMETERS]'))
    print_formatted_text('')

    print_formatted_text(HTML('<b>Objects</b>'))
    for o in object_types:
        print_formatted_text(HTML('<b>%s</b>' % o))
    print_formatted_text('')

    print_formatted_text(HTML('<b>Commands</b>'))
    for c in commands:
        print_formatted_text((HTML('<b>%s</b>' % c)))
    print_formatted_text('')

    print_formatted_text(HTML('<b>Misc</b>'))
    for m in misc:
        print_formatted_text((HTML('<b>%s</b>' % m)))


async def cmd_app_list(tlwbe: Tlwbe, parameters: dict):
    apps = await tlwbe.list_apps()
    if apps.code == RESULT_OK:
        for eui in apps.eui_list:
            app = await tlwbe.get_app_by_eui(eui)
            if app.code == RESULT_OK:
                print_formatted_text(HTML('<b>%s</b>[%s]' % (app.app.name, app.app.eui)))


async def cmd_app_add(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.add_app(name=parameters['name'])
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_app_get(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.get_app_by_eui(eui=parameters['eui'])
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_app_delete(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.delete_app(eui=parameters['eui'])
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_dev_list(tlwbe: Tlwbe, parameters: dict):
    devs = await tlwbe.list_devs()
    if devs.code == RESULT_OK:
        for eui in devs.eui_list:
            dev = await tlwbe.get_dev_by_eui(eui)
            if dev.code == RESULT_OK:
                app = await tlwbe.get_app_by_eui(eui)
                if app.code == RESULT_OK:
                    print_formatted_text(
                        HTML('<b>%s</b>[%s]' % (
                            dev.dev.name, dev.dev.eui)))  # app.app.name, app.app.eui)))


async def cmd_dev_add(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.add_dev(name=parameters['name'], app_eui=parameters['app_eui'])
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_dev_delete(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.delete_dev(eui=parameters['eui'])
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_uplink_list(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.list_uplinks(app_eui=parameters.get('app_eui'),
                                      dev_eui=parameters.get('dev_eui'))
    if result.code is not 0:
        print_formatted_text('failed')
        return

    for uplink in result.uplinks:
        hexbytes = []

        printable_chars = bytes(string.printable, 'ascii')
        asciibytes = []
        for b in uplink.payload:
            hexbytes.append('%02x' % b)
            if b in printable_chars:
                asciibytes.append('%c' % b)
            else:
                asciibytes.append('.')
        print_formatted_text('%s %s@%s %s [%s]' % (
            uplink.timestamp, uplink.dev_eui, uplink.port, ''.join(hexbytes), ''.join(asciibytes)))


async def cmd_downlink_add(tlwbe: Tlwbe, parameters: dict):
    payload = b'poop'
    result = await tlwbe.send_downlink(parameters['app_eui'], parameters['dev_eui'], int(parameters['port']), payload)
    if result.code is not 0:
        print_formatted_text('failed')


async def cmd_downlink_list(tlwbe: Tlwbe, parameters: dict):
    result = await tlwbe.list_downlinks(app_eui=parameters.get('app_eui'),
                                        dev_eui=parameters.get('dev_eui'))


class Command:
    __slots__ = ['func', 'required_fields', 'optional_fields']

    def __init__(self, func: callable, required_fields: list = None, optional_fields: list = None):
        self.func = func
        self.required_fields = required_fields
        self.optional_fields = optional_fields


class Object:
    __slots__ = ['commands']

    def __init__(self, get: callable = None, get_required_fields: list = None, get_optional_fields: list = None,
                 add: callable = None, add_required_fields: list = None, add_optional_fields: list = None,
                 delete: callable = None, delete_required_fields: list = None, delete_optional_fields: list = None,
                 list: callable = None, list_required_fields: list = None, list_optional_fields: list = None):
        self.commands = {
            'get': Command(get, get_required_fields, get_optional_fields),
            'add': Command(add, add_required_fields, add_optional_fields),
            'del': Command(delete, delete_required_fields, delete_optional_fields),
            'list': Command(list, list_required_fields, list_optional_fields)
        }


FIELD_NAME = 'name'
FIELD_EUI = 'eui'
FIELD_APP_EUI = 'app_eui'
FIELD_DEV_EUI = 'dev_eui'
FIELD_PORT = 'port'

objects = {
    'app': Object(get=cmd_app_get, get_required_fields=[FIELD_EUI],
                  add=cmd_app_add, add_required_fields=[FIELD_NAME],
                  delete=cmd_app_delete, delete_required_fields=[FIELD_EUI],
                  list=cmd_app_list),
    'dev': Object(add=cmd_dev_add, add_required_fields=[FIELD_NAME, FIELD_APP_EUI],
                  delete=cmd_dev_delete, delete_required_fields=[FIELD_EUI],
                  list=cmd_dev_list),
    'uplink': Object(list=cmd_uplink_list),
    'downlink': Object(add=cmd_downlink_add, add_required_fields=[FIELD_APP_EUI, FIELD_DEV_EUI, FIELD_PORT],
                       list=cmd_downlink_list)
}
object_types = list(objects.keys())
commands = ['get', 'add', 'del', 'update', 'list', 'help']
misc = ['help']
fields = [FIELD_NAME, FIELD_EUI, FIELD_APP_EUI, FIELD_DEV_EUI, FIELD_PORT]

parameter_regex = '(%s)\\s(\\w{1,})\\s?' % '|'.join(fields)
regex = '(%s)(\\s(%s)\\s?)?((%s){0,})' % ('|'.join(misc + object_types), '|'.join(commands), parameter_regex)
print(regex)


async def main(host: str, port: int):
    tlwbe = Tlwbe(host, port)

    print('waiting for connection to broker...')
    await tlwbe.wait_for_connection()

    session = PromptSession(completer=Completer(), lexer=Lexer())

    print_formatted_text(HTML("type <b>help</b> if you're confused... <b>ctrl-c</b> to quit"))

    while True:
        with patch_stdout():
            result: str = await session.prompt('tlwbe> ', async_=True)

            call = True

            matches = re.search(regex, result)
            if matches is not None:
                obj = matches.group(1)
                command = matches.group(3)
                if command is None:
                    command = 'list'
                raw_parameters = matches.group(4)
                matches = re.finditer(parameter_regex, raw_parameters)
                parameters = {}
                for match in matches:
                    parameters[match.group(1)] = match.group(2)

                print('%s %s %s' % (obj, command, str(parameters)))

                o = objects.get(obj)
                c = o.commands.get(command)

                if c is not None:
                    if c.required_fields is not None:
                        missing_fields = c.required_fields.copy()
                        for f in parameters:
                            if f in missing_fields:
                                missing_fields.remove(f)
                        if len(missing_fields) is not 0:
                            print_formatted_text("One or more required parameters are missing...")
                            for f in missing_fields:
                                print_formatted_text(HTML('parameter <b>%s</b> is required' % f))
                            call = False

                    if call:
                        try:
                            await c.func(tlwbe, parameters)
                        except asyncio.TimeoutError:
                            print_formatted_text('timeout :(')

            else:
                cmd_unknown()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='tlwbe client')
    parser.add_argument('--host', type=str, default='localhost')
    parser.add_argument('--port', type=int)
    args = parser.parse_args()

    try:
        asyncio.get_event_loop().run_until_complete(main(args.host, args.port))
    except KeyboardInterrupt:
        print('bye bye')
