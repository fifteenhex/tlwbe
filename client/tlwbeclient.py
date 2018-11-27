#!/usr/bin/env python3


from cmd import Cmd
from uuid import uuid4
from rxmqtt import RxMqttClient
import json
from rx import Observable

rxmqttclient = RxMqttClient("espressobin1")


def publishandwaitforresult(topic, payload={}):
    token = str(uuid4())
    payload['token'] = token
    rxmqttclient.mqttclient.publish(topic, json.dumps(payload))
    return rxmqttclient.publishsubject \
        .filter(lambda msg: msg.get('payload').get('token') == token) \
        .first()


def publishandwaitforresult_newstyle(topic, payload={}):
    token = str(uuid4())
    rxmqttclient.mqttclient.publish("%s/%s" % (topic, token), json.dumps(payload))
    return rxmqttclient.publishsubject \
        .first()


class Interpreter(Cmd):
    def do_dev(self, s):
        devs = publishandwaitforresult("tlwbe/control/dev/list") \
            .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
            .to_blocking()

        for d in devs:
            payload = {'eui': d}
            dev = publishandwaitforresult("tlwbe/control/dev/get", payload) \
                .to_blocking()
            for dd in dev:
                print(dd)

            payload = {'deveui': d}
            uplinks = publishandwaitforresult_newstyle("tlwbe/uplinks/query", payload) \
                .flat_map(lambda msg: Observable.from_(msg['payload']['uplinks'])) \
                .to_blocking()
            print("timestamp\tport\tpayload")
            for u in uplinks:
                print("%s\t%s\t%s" % (u['timestamp'], u['port'], u['payload']))

    def help_dev(self):
        pass

    def do_app(self, s):
        apps = publishandwaitforresult("tlwbe/control/app/list") \
            .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
            .to_blocking()

        for a in apps:
            payload = {'eui': a}
            app = publishandwaitforresult("tlwbe/control/app/get", payload) \
                .to_blocking()
            for aa in app:
                print(aa)

    def help_app(self):
        pass


if __name__ == '__main__':
    rxmqttclient.mqttclient.subscribe("tlwbe/control/result")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplink/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplinks/result/#")

    interpreter = Interpreter()
    interpreter.cmdloop()
