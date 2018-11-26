#!/usr/bin/env python3


from cmd import Cmd
from uuid import uuid4
from rxmqtt import RxMqttClient
import json
from rx import Observable

rxmqttclient = RxMqttClient("espressobin1")


class Interpreter(Cmd):
    def do_dev(self, s):
        token = str(uuid4())
        payload = {"token": token}
        rxmqttclient.mqttclient.publish("tlwbe/control/dev/list", json.dumps(payload))
        devs = rxmqttclient.publishsubject \
            .filter(lambda msg: msg.get('payload').get('token') == token) \
            .first() \
            .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
            .to_blocking()

        for d in devs:
            print(d)
            token = str(uuid4())
            payload = {"token": token, 'eui': d}
            rxmqttclient.mqttclient.publish("tlwbe/control/dev/get", json.dumps(payload))
            dev = rxmqttclient.publishsubject \
                .filter(lambda msg: msg.get('payload').get('token') == token) \
                .first() \
                .to_blocking()

            for dd in dev:
                print(dd)

    def help_dev(self):
        pass

    def do_app(self, s):
        token = str(uuid4())
        payload = {"token": token}
        rxmqttclient.mqttclient.publish("tlwbe/control/app/list", json.dumps(payload))
        apps = rxmqttclient.publishsubject \
            .filter(lambda msg: msg.get('payload').get('token') == token) \
            .flat_map(lambda msg: Observable.from_(msg['payload']['result'])) \
            .first() \
            .to_blocking()

        for a in apps:
            print(a)

    def help_app(self):
        pass


if __name__ == '__main__':
    rxmqttclient.mqttclient.subscribe("tlwbe/control/result")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplink/#")
    rxmqttclient.mqttclient.subscribe("tlwbe/uplinks/result/#")

    interpreter = Interpreter()
    interpreter.cmdloop()
