# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0

import time as t
import json
import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERT, PATH_TO_KEY, PATH_TO_ROOT, MESSAGE, TOPIC, and RANGE
ENDPOINT = "ENDPOINT.iot.us-east-2.amazonaws.com"
CLIENT_ID = "THINGNAME"
PATH_TO_CERT = "../data/FILENAME.pem.crt"
PATH_TO_KEY = "../data/FILENAME.pem.key"
PATH_TO_ROOT = "../data/FILENAME.pem"
MESSAGE = {"state":{"reported":{"temp":23.54,"pressure":997.5694,"humidity":55.86035,"wind_direction":"NW","wind_speed":4.5,"rainfall_mm":2.3,"battery":3.902637,"timestamp":1605071283}}}
TOPIC = "THINGNAME/weather"
RANGE = 10

myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient(CLIENT_ID)
myAWSIoTMQTTClient.configureEndpoint(ENDPOINT, 8883)
myAWSIoTMQTTClient.configureCredentials(PATH_TO_ROOT, PATH_TO_KEY, PATH_TO_CERT)

myAWSIoTMQTTClient.connect()
print('Begin Publish')
for i in range (RANGE):
    MESSAGE["state"]["reported"]["timestamp"] += 111 * i
    myAWSIoTMQTTClient.publish(TOPIC, json.dumps(MESSAGE), 1)
    print("Published: '" + json.dumps(MESSAGE) + "' to the topic: " + "'test/testing'")
    t.sleep(0.1)
print('Publish End')
myAWSIoTMQTTClient.disconnect()
