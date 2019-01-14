#!/usr/bin/env python3

import argparse
from tlwpy.tlwbe import Tlwbe, RESULT_OK
import asyncio
from prompt_toolkit import PromptSession, print_formatted_text, HTML
from prompt_toolkit.eventloop import use_asyncio_event_loop
from prompt_toolkit.patch_stdout import patch_stdout
from prompt_toolkit.document import Document
import prompt_toolkit.lexers
import prompt_toolkit.completion
import prompt_toolkit.contrib.regular_languages.compiler
import re

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


async def cmd_uplink(tlwbe: Tlwbe, parameters: dict):
    pass


async def cmd_downlink(tlwbe: Tlwbe, parameters: dict):
    pass


class Object:
    __slots__ = ['commands', 'required_fields', 'optional_fields']

    def __init__(self, get: callable = None,
                 add: callable = None, add_required_fields: list = None, add_optional_fields: list = None,
                 delete: callable = None, list: callable = None):
        self.commands = {
            'get': get,
            'add': add,
            'delete': delete,
            'list': list
        }
        self.required_fields = {
            'add': add_required_fields
        }
        self.optional_fields = {
            'add': add_optional_fields
        }


objects = {
    'app': Object(add=cmd_app_add, add_required_fields=['name'], list=cmd_app_list),
    'dev': Object(add=cmd_dev_add, add_required_fields=['name', 'app_eui'], list=cmd_dev_list),
    'uplink': Object(cmd_uplink),
    'downlink': Object(cmd_downlink)
}
object_types = list(objects.keys())
commands = ['get', 'add', 'del', 'update', 'list', 'help']
misc = ['help']
fields = ['name', 'eui', 'app_eui']

parameter_regex = '(%s)\\s(\\w{1,})\\s?' % '|'.join(fields)
regex = '(%s)(\\s(%s)\\s?)?((%s){0,})' % (
    '|'.join(misc + object_types), '|'.join(commands), parameter_regex)
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

                # print('%s %s %s' % (obj, command, str(parameters)))

                o = objects.get(obj)
                c = o.commands.get(command)
                r_f: list = o.required_fields.get(command)
                o_f = o.optional_fields.get(command)

                if c is not None:
                    if r_f is not None:
                        for f in parameters:
                            if f in r_f:
                                r_f.remove(f)
                        if len(r_f) is not 0:
                            print_formatted_text("One or more required parameters are missing...")
                            for f in r_f:
                                print_formatted_text(HTML('parameter <b>%s</b> is required' % f))
                            call = False

                    if call:
                        await c(tlwbe, parameters)
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
