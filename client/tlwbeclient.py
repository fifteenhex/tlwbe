#!/usr/bin/env python3


import cmd2
import argparse
from uuid import uuid4
from rxmqtt import RxMqttClient
import json
from rx import Observable

rxmqttclient = RxMqttClient("espressobin1")


def publishandwaitforresult_newstyle(topic, payload={}):
    token = str(uuid4())
    rxmqttclient.mqttclient.publish("%s/%s" % (topic, token), json.dumps(payload))
    return rxmqttclient.publishsubject \
        .first()


class Interpreter(cmd2.Cmd):
    intro = "tlwbe client, type help if you're confused"
    prompt = "> "

    actions = ["list", "get", "add", "update", "delete"]

    dev_parser = argparse.ArgumentParser()
    dev_parser.add_argument('--action', type=str, choices=actions, default="list")

    @cmd2.with_argparser(dev_parser)
    def do_dev(self, args):
        if args.action == 'list':
            devs = publishandwaitforresult_newstyle("tlwbe/control/dev/list") \
                .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
                .to_blocking()

            print("name\t\tdev_eui\t\tapp_eui\t\tkey")
            for d in devs:
                payload = {'eui': d}
                dev = publishandwaitforresult_newstyle("tlwbe/control/dev/get", payload) \
                    .to_blocking()
                for dd in dev:
                    devdata = dd['payload']['dev']
                    print("%s\t\t%s\t\t%s\t\t%s" % (devdata['name'], devdata['eui'], devdata['appeui'], devdata['key']))

    app_parser = argparse.ArgumentParser()
    app_parser.add_argument('--action', type=str, choices=actions, default="list")

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

    def do_uplink(self, s):
        # payload = {'deveui': d}
        # uplinks = publishandwaitforresult_newstyle("tlwbe/uplinks/query", payload) \
        #    .flat_map(lambda msg: Observable.from_(msg['payload']['uplinks'])) \
        #    .to_blocking()
        # print("timestamp\tport\tpayload")
        # for u in uplinks:
        #    print("%s\t%s\t%s" % (u['timestamp'], u['port'], u['payload']))
        pass

    def help_uplink(self):
        pass

    def do_downlink(self, s):
        # payload = {'deveui': d}
        # uplinks = publishandwaitforresult_newstyle("tlwbe/uplinks/query", payload) \
        #    .flat_map(lambda msg: Observable.from_(msg['payload']['uplinks'])) \
        #    .to_blocking()
        # print("timestamp\tport\tpayload")
        # for u in uplinks:
        #    print("%s\t%s\t%s" % (u['timestamp'], u['port'], u['payload']))
        pass

    def help_downlink(self):
        pass


if __name__ == '__main__':
    rxmqttclient.mqttclient.subscribe("tlwbe/control/result/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplink/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplinks/result/#")

    interpreter = Interpreter()
    interpreter.cmdloop()
