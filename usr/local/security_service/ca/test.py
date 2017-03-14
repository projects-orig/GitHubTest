#!/usr/bin/env python

import paho.mqtt.client as mqtt


def on_connect(mq, userdata, rc, _):
    # subscribe when connected.
    print("on_connect")
    mq.subscribe('#', qos=2)


def on_message(mq, userdata, msg):
    print("topic: %s" % msg.topic)
    print("payload: %s" % msg.payload)
    print("qos: %d" % msg.qos)


client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

user = "amigo"
password = "swetop"
client.username_pw_set(user, password)

client.tls_set("./rootCA.pem",
               "./client.crt",
               "./client.key")
# disables peer verification
# client.tls_insecure_set(True)
client.connect("fs1p.etopnetwork.com.tw", 8883, 60)

client.loop_forever()
