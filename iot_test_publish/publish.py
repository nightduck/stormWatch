# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0

import time as t
import json
import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERT, PATH_TO_KEY, PATH_TO_ROOT, MESSAGE, TOPIC, and RANGE
ENDPOINT = "a2hdizbqiesak7-ats.iot.us-east-2.amazonaws.com"
CLIENT_ID = "testDevice"
PATH_TO_CERT = "certificates/29707f2f05-certificate.pem.crt"
PATH_TO_KEY = "certificates/29707f2f05-private.pem.key"
PATH_TO_ROOT = "certificates/root.pem"
MESSAGE = "Hello World! This is our very first IoT test message!"
TOPIC = "test/testing"
RANGE = 10

myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient(CLIENT_ID)
myAWSIoTMQTTClient.configureEndpoint(ENDPOINT, 8883)
myAWSIoTMQTTClient.configureCredentials(PATH_TO_ROOT, PATH_TO_KEY, PATH_TO_CERT)

myAWSIoTMQTTClient.connect()
print('Begin Publish')
for i in range (RANGE):
    data = "{} [{}]".format(MESSAGE, i+1)
    message = {"message" : data}
    myAWSIoTMQTTClient.publish(TOPIC, json.dumps(message), 1)
    print("Published: '" + json.dumps(message) + "' to the topic: " + "'test/testing'")
    t.sleep(0.1)
print('Publish End')
myAWSIoTMQTTClient.disconnect()
