import paho.mqtt.client as mqtt
from rx.subjects import Subject
import json


class RxMqttClient:
    mqttclient = mqtt.Client()
    publishsubject = Subject()

    def __on_message(self, client, userdata, msg):
        payloadjson = msg.payload.decode("utf-8")
        payload = json.loads(payloadjson)
        parsedmsg = {'topic': msg.topic, 'payload': payload}
        userdata.publishsubject.on_next(parsedmsg)

    def __init__(self, hostname):
        self.mqttclient.user_data_set(self)
        self.mqttclient.on_message = self.__on_message
        self.mqttclient.connect(hostname)
        self.mqttclient.loop_start()
