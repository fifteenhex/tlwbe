#!/usr/bin/env python3


import cmd2
import argparse
from uuid import uuid4
from rxmqtt import RxMqttClient
import json
from rx import Observable
import datetime
import base64
import binascii

rxmqttclient = RxMqttClient("espressobin1")


def publishandwaitforresult_newstyle(topic, payload={}):
    # this seems to be asking for a race condition
    token = str(uuid4())
    rxmqttclient.mqttclient.publish("%s/%s" % (topic, token), json.dumps(payload))
    return rxmqttclient.publishsubject \
        .filter(lambda msg: msg['topic'].split('/')[-1] == token) \
        .first()


class Interpreter(cmd2.Cmd):
    intro = "tlwbe client, type help if you're confused"
    prompt = "> "

    actions = ["list", "get", "add", "update", "delete"]

    def __adddnamearg(parser):
        parser.add_argument('--name', type=str, nargs='?')

    def __adddevuiarg(parser):
        parser.add_argument('--deveui', type=str, nargs='?')

    def __addappeuiarg(parser):
        parser.add_argument('--appeui', type=str, nargs='?')

    dev_parser = argparse.ArgumentParser()
    dev_parser.add_argument('--action', type=str, nargs='?', choices=actions, default="list")
    __adddnamearg(dev_parser)
    __adddevuiarg(dev_parser)
    __addappeuiarg(dev_parser)

    def __printdev(self, dev):
        print("%s\t\t%s\t\t%s\t\t%s" % (dev['name'], dev['eui'], dev['appeui'], dev['key']))

    @cmd2.with_argparser(dev_parser)
    def do_dev(self, args):
        if args.action == 'list':
            devs = publishandwaitforresult_newstyle("tlwbe/control/dev/list") \
                .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
                .to_blocking()

            print("name\t\tdev_eui\t\tapp_eui")
            for d in devs:
                payload = {'eui': d}
                dev = publishandwaitforresult_newstyle("tlwbe/control/dev/get", payload) \
                    .to_blocking()
                for dd in dev:
                    devdata = dd['payload']['dev']
                    print("%s\t\t%s\t\t%s" % (devdata['name'], devdata['eui'], devdata['appeui']))
        elif args.action == 'get':
            if args.deveui is None:
                print("need a dev eui buddy")
                return

            payload = {'eui': args.deveui}
            dev = publishandwaitforresult_newstyle("tlwbe/control/dev/get", payload) \
                .to_blocking()
            for dd in dev:
                devdata = dd['payload']['dev']
                self.__printdev(devdata)
        elif args.action == 'add':
            if args.appeui is None or args.name is None:
                print('need a name and an app eui')
                return

            payload = {'name': args.name, 'appeui': args.appeui}
            if args.deveui is not None:
                payload['eui'] = args.deveui

            result = publishandwaitforresult_newstyle("tlwbe/control/dev/add", payload) \
                .to_blocking()
            for rr in result:
                appdata = rr['payload']
                print("%s" % str(appdata))
        else:
            print("%s isn't implemented yet" % args.action)

    app_parser = argparse.ArgumentParser()
    app_parser.add_argument('--action', type=str, nargs='?', choices=actions, default="list")
    __adddnamearg(app_parser)
    __addappeuiarg(app_parser)

    @cmd2.with_argparser(app_parser)
    def do_app(self, args):
        if args.action == 'list':
            apps = publishandwaitforresult_newstyle("tlwbe/control/app/list") \
                .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
                .to_blocking()

            print("name\t\teui")
            for a in apps:
                payload = {'eui': a}
                app = publishandwaitforresult_newstyle("tlwbe/control/app/get", payload) \
                    .to_blocking()
                for aa in app:
                    print("%s\t\t%s" % (aa['payload']['app']['name'], aa['payload']['app']['eui']))
        elif args.action == 'get':
            if args.appeui is None:
                print("need an app eui buddy")
                return

            payload = {'eui': args.appeui}
            app = publishandwaitforresult_newstyle("tlwbe/control/app/get", payload) \
                .to_blocking()
            for aa in app:
                appdata = aa['payload']['app']
                print("%s\t\t%s" % (appdata['name'], appdata['eui']))
        elif args.action == 'add':
            if args.appeui is None or args.name is None:
                print('need a name and a eui')
                return

            payload = {'name': args.name, 'eui': args.appeui}
            result = publishandwaitforresult_newstyle("tlwbe/control/app/add", payload) \
                .to_blocking()
            for rr in result:
                appdata = rr['payload']
                print("%s" % str(appdata))
        else:
            print("%s isn't implemented yet" % args.action)

    uplink_parser = argparse.ArgumentParser()
    __adddevuiarg(uplink_parser)
    __addappeuiarg(uplink_parser)

    @cmd2.with_argparser(uplink_parser)
    def do_uplink(self, args):
        if args.deveui is None and args.appeui is None:
            print("a dev eui or an app eui is required")
            return

        payload = {}
        if args.deveui is not None:
            payload['deveui'] = args.deveui

        uplinks = publishandwaitforresult_newstyle("tlwbe/uplinks/query", payload) \
            .flat_map(lambda msg: Observable.from_(msg['payload']['uplinks'])) \
            .to_blocking()
        print("timestamp\tport\tpayload")
        for u in uplinks:
            print("%s\t%s\t%s" % (u['timestamp'], u['port'], u['payload']))

    downlink_parser = argparse.ArgumentParser()
    __adddevuiarg(downlink_parser)
    __addappeuiarg(downlink_parser)

    @cmd2.with_argparser(downlink_parser)
    def do_downlink(self, args):
        if args.deveui is None or args.appeui is None:
            print("an app eui and dev eui is required")
            return

        pl = base64.b64encode(b'omnomnom').decode('ascii')
        msgjson = {'payload': pl}

        result = publishandwaitforresult_newstyle("tlwbe/downlink/schedule/%s/%s/%d" % (args.appeui, args.deveui, 1),
                                                  msgjson) \
            .flat_map(lambda msg: Observable.from_(msg['payload']['uplinks'])) \
            .to_blocking()
        print("timestamp\tport\tpayload")
        for u in result:
            print("%s\t%s\t%s" % (u['timestamp'], u['port'], u['payload']))

    watch_parser = argparse.ArgumentParser()

    @cmd2.with_argparser(watch_parser)
    def do_watch(self, args):
        #            .do_action(lambda msg: print(msg)) \
        msgs = rxmqttclient.publishsubject \
            .to_blocking()
        for msg in msgs:
            topicparts = msg['topic'].split('/')
            payload = msg['payload']
            interface = topicparts[1]
            ts = datetime.datetime.utcfromtimestamp(payload['timestamp'] / 1000000)
            if interface == "join":
                print("%s - %s@%s joined the party" % (str(ts), topicparts[-1], topicparts[-2]))
            elif interface == "uplink":
                deveui = payload['deveui']
                appeui = payload['appeui']
                data = base64.b64decode(payload['payload'])
                hexeddata = binascii.hexlify(data)
                print("%s - %s@%s:%d -> %s" % (str(ts), deveui, appeui, payload['port'], hexeddata))


if __name__ == '__main__':
    rxmqttclient.mqttclient.subscribe("tlwbe/control/result/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplink/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplinks/result/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/join/+/+")

    interpreter = Interpreter()
    interpreter.cmdloop()
