# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
# SPDX-License-Identifier: MIT-0

import time as t
import json
import random
import AWSIoTPythonSDK.MQTTLib as AWSIoTPyMQTT
import sys

if len(sys.argv) != 3:
    print("Usage: python push_config.py nodename config.json")
    exit()

# Define ENDPOINT, CLIENT_ID, PATH_TO_CERT, PATH_TO_KEY, PATH_TO_ROOT, MESSAGE, TOPIC, and RANGE
ENDPOINT = "a2hdizbqiesak7-ats.iot.us-east-2.amazonaws.com"
CLIENT_ID = sys.argv[1]
PATH_TO_CERT = "../data/29707f2f05-certificate.pem.crt"
PATH_TO_KEY = "../data/29707f2f05-private.pem.key"
PATH_TO_ROOT = "../data/root.pem"
CONFIG_FILE = str(sys.argv[2])
TOPIC = "config/" + str(sys.argv[1])
RANGE = 10

myAWSIoTMQTTClient = AWSIoTPyMQTT.AWSIoTMQTTClient(CLIENT_ID)
myAWSIoTMQTTClient.configureEndpoint(ENDPOINT, 8883)
myAWSIoTMQTTClient.configureCredentials(PATH_TO_ROOT, PATH_TO_KEY, PATH_TO_CERT)

fin = open(CONFIG_FILE, 'r')
myAWSIoTMQTTClient.connect()
print('Begin Publish')
content = fin.read()
myAWSIoTMQTTClient.publish(TOPIC, content, 1)
print("Published: '" + json.dumps(content) + "' to the topic: " + TOPIC)
print('Publish End')
myAWSIoTMQTTClient.disconnect()
