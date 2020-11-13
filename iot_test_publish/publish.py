# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0

import time as t
import json
import random
import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERT, PATH_TO_KEY, PATH_TO_ROOT, MESSAGE, TOPIC, and RANGE
ENDPOINT = "a2hdizbqiesak7-ats.iot.us-east-2.amazonaws.com"
CLIENT_ID = "node02"
PATH_TO_CERT = "../data/29707f2f05-certificate.pem.crt"
PATH_TO_KEY = "../data/29707f2f05-private.pem.key"
PATH_TO_ROOT = "../data/root.pem"
MESSAGE = {"state":{"reported":{"power":0,"timestamp":1605071283,"node":"node02"}}}
TOPIC = "sensor/node02/lightning"
RANGE = 10

myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient(CLIENT_ID)
myAWSIoTMQTTClient.configureEndpoint(ENDPOINT, 8883)
myAWSIoTMQTTClient.configureCredentials(PATH_TO_ROOT, PATH_TO_KEY, PATH_TO_CERT)

myAWSIoTMQTTClient.connect()
print('Begin Publish')
for i in range (RANGE):
    MESSAGE["state"]["reported"]["timestamp"] = int(t.time())
    MESSAGE["state"]["reported"]["power"] = random.randint(1000, 50000)
    myAWSIoTMQTTClient.publish(TOPIC, json.dumps(MESSAGE), 1)
    print("Published: '" + json.dumps(MESSAGE) + "' to the topic: " + TOPIC)
    t.sleep(30)
print('Publish End')
myAWSIoTMQTTClient.disconnect()
