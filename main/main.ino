#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "SPIFFS.h"
#include "Adafruit_BME280.h"
#include "SparkFun_AS3935.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define COUNTER_0 25
#define COUNTER_1 26
#define COUNTER_2 12
#define COUNTER_3 27
#define COUNTER_4 33
#define COUNTER_5 15
#define COUNTER_6 32
#define COUNTER_7 14
#define COUNTER_RST 34
#define LIGHTN_INT 4
#define ANEM_INT 36
#define WEATHER_VANE 39 
#define LIGHTN_CS 21     // Bogus pin, since CS is tied low
//#define FACTORY_RESET 21
#define BATT 35
#define LED 13
#define SEALEVELPRESSURE_HPA 1013.25
#define FLASH_START 32
#define CONFIG_ADDR 0

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

#define CONFIG_FILENAME "/config.json"
#define WEATHER_FILENAME "/weather_readings"
#define LIGHTNING_FILENAME "/lightning_events"

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "$aws/things/storm_watch/shadow/update"
#define AWS_IOT_SUBSCRIBE_TOPIC "$aws/things/storm_watch/shadow/update"

typedef struct weather_reading {
  float temp;
  float pressure;
  float humidity;
  int wind_direction;
  float wind_speed;
  float rainfall_mm;
} weather_reading_t;

typedef struct lightn_reading {
  int power;
  time_t timestamp;
} lightn_event_t;

StaticJsonDocument<768> config;
bool online;

Adafruit_BME280 bme;
SparkFun_AS3935 lightning;
bool sawLightning;
bool factoryReset;

char AWS_CERT_CA[1280];
char AWS_CERT_CRT[1280];
char AWS_CERT_PRIVATE[1792];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void messageHandler(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

//  StaticJsonDocument<200> doc;
//  deserializeJson(doc, payload);
//  const char* message = doc["message"];
}

void connectToWiFi()
{
  //WiFi.mode(WIFI_STA);
  // TODO: Error handling with the keys. Fallback to offline mode if failed
  WiFi.begin(config["wifi_ssid"].as<const char *>(), config["wifi_password"].as<const char *>());

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");
}

void connectAWS()
{
  // Load in AWS certs and keys from flash
  Serial.println("Reading certs from memory");
  File certCA = SPIFFS.open(config["aws_cert_ca"].as<const char*>(), FILE_READ);
  File certCRT = SPIFFS.open(config["aws_cert_crt"].as<const char*>(), FILE_READ);
  File certPrivate = SPIFFS.open(config["aws_cert_private"].as<const char*>(), FILE_READ);
  if (!certCA || !certCRT || !certPrivate) {
    Serial.println("HALTING!!! AWS files not found in flash! Can't find: ");
    Serial.println(config["aws_cert_ca"].as<const char*>());
    Serial.println(config["aws_cert_crt"].as<const char*>());
    Serial.println(config["aws_cert_private"].as<const char*>());
    while(true);
  }

  String temp = "";
  while (certCA.available()) {
    temp += certCA.readString();
  }
  strcpy(AWS_CERT_CA, temp.c_str());

  temp = "";
  while (certCRT.available()) {
    temp += certCRT.readString();
  }
  strcpy(AWS_CERT_CRT, temp.c_str());

  temp = "";
  while (certPrivate.available()) {
    temp += certPrivate.readString();
  }
  strcpy(AWS_CERT_PRIVATE, temp.c_str());

  certCA.close();
  certCRT.close();
  certPrivate.close();

  connectToWiFi();
  
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(config["aws_iot_endpoint"].as<const char*>(), 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(config["thingname"].as<const char*>())) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

int read_counter() {
  char count = digitalRead(COUNTER_0);
  count += digitalRead(COUNTER_1) << 1;
  count += digitalRead(COUNTER_2) << 2;
  count += digitalRead(COUNTER_3) << 3;
  count += digitalRead(COUNTER_4) << 4;
  count += digitalRead(COUNTER_5) << 5;
  count += digitalRead(COUNTER_6) << 6;
  count += digitalRead(COUNTER_7) << 7;

  return count;
}

void reset_counter() {
  digitalWrite(COUNTER_RST, HIGH);
  delay(2);
  digitalWrite(COUNTER_RST, LOW);
}

float readBattery() {
  float battLvl = 6.6 * analogRead(BATT) / 4096;
  return battLvl;
}

void print_hw_debug() {
  Serial.print("Free heap: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("Free psram: ");
  Serial.println(ESP.getFreePsram());
  Serial.print("Free sketch space: ");
  Serial.println(ESP.getFreeSketchSpace());
  Serial.print("Sketch size: ");
  Serial.println(ESP.getSketchSize());
  Serial.print("Flash size: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("Battery Level: ");
  Serial.println(readBattery());
}

bool record_weather_reading(const weather_reading_t *reading) {
  // If in online mode, publish to cloud
  if (online) {
    StaticJsonDocument<256> jsonDoc;
    JsonObject stateObj = jsonDoc.createNestedObject("state");
    JsonObject reportedObj = stateObj.createNestedObject("reported");
    reportedObj["temp"] = reading->temp;
    reportedObj["pressure"] = reading->pressure;
    reportedObj["humidity"] = reading->humidity;
    reportedObj["wind_direction"] = reading->wind_direction;
    reportedObj["wind_speed"] = reading->wind_speed;
    reportedObj["rainfall_mm"] = reading->rainfall_mm;
    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);
    Serial.println(jsonBuffer);

    if(!client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer)) {
      online = false;
    } else
    {
      Serial.println("Published reading");
      return true;
    }
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  if (!online) {
    File weather = SPIFFS.open(WEATHER_FILENAME, FILE_APPEND);
    if (!weather) {
      return false;
    }
    if (weather.write((uint8_t*)reading, sizeof(weather_reading_t)) < sizeof(weather_reading_t)) {
      return false;
    }

    Serial.println("Saved reading to flash");
    online = true;
    return true;
  }

  Serial.println("How did you manage to get here???");
  return false;
}

bool record_lightning_event(const lightn_event_t *event) {
  // If in online mode, publish to cloud
  if (online) {
    online = false; // TODO: Implement online features, for now: fail
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  if (!online) {
    File lightning = SPIFFS.open(WEATHER_FILENAME, FILE_APPEND);
    if (!lightning) {
      return false;
    }
    if (lightning.write((uint8_t*)event, sizeof(lightn_event_t)) < sizeof(lightn_event_t)) {
      return false;
    }

    return true;
  }

  return false; // IDK how you could possibly reach here
}

bool publish_backlog() {
  Serial.println("Inspecting offline backlog");

  File weather = SPIFFS.open(WEATHER_FILENAME, FILE_READ);
  File lightning = SPIFFS.open(LIGHTNING_FILENAME, FILE_READ);
  if (!weather || !lightning) {
    Serial.println("Unable to open config files");
    return false;
  }
  
  Serial.print("Weather readings: ");
  Serial.println(weather.size() / sizeof(weather_reading_t));
  Serial.print("Lightning events: ");
  Serial.println(lightning.size() / sizeof(lightn_event_t));
  Serial.println();

  weather_reading_t reading;
  while(weather.read((uint8_t*)&reading, sizeof(weather_reading_t)) != 0) {
    record_weather_reading(&reading);
  };

  lightn_event_t event;
  while(lightning.read((uint8_t*)&event, sizeof(lightn_event_t)) != 0) {
    // TODO: Replace this with WiFi upload
    Serial.print("Lightning energy = ");
    Serial.print(event.power);

    Serial.print("Timestamp = ");
    Serial.print(event.timestamp);
  }

  SPIFFS.remove(WEATHER_FILENAME);
  SPIFFS.remove(LIGHTNING_FILENAME);
  return true;
}

void take_reading() {
  weather_reading_t reading;
  reading.temp = bme.readTemperature();
  reading.pressure = bme.readPressure() / 100.0F;
  reading.humidity = bme.readHumidity();

  if (!record_weather_reading(&reading)) {
    Serial.println("FAILED TO SAVE DATA. ABORTING");
    print_hw_debug();
    while(true);
  };
}

void IRAM_ATTR LIGHTN_ISR() {
  sawLightning = true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("Serial initialized");

  // Serial.println("Awaken get ready...");
  // for(int i = 0; i < 10; i++) {
  //   Serial.println(i);
  //   delay(1000);
  // }

  pinMode(LED, OUTPUT);
  pinMode(COUNTER_0, INPUT);
  pinMode(COUNTER_1, INPUT);
  pinMode(COUNTER_2, INPUT);
  pinMode(COUNTER_3, INPUT);
  pinMode(COUNTER_4, INPUT);
  pinMode(COUNTER_5, INPUT);
  pinMode(COUNTER_6, INPUT);
  pinMode(COUNTER_7, INPUT);
  pinMode(COUNTER_RST, OUTPUT);
  pinMode(LIGHTN_INT, INPUT);
  //pinMode(FACTORY_RESET, INPUT);
  online = true;

  if (!SPIFFS.begin(true)) {
    Serial.println("Could not initialize SPIFFS");
    while (true);
  } else {
    Serial.print("Size of entire filesystem: ");
    Serial.println(SPIFFS.totalBytes());
    Serial.print("Used: ");
    Serial.println(SPIFFS.usedBytes());
  }

  // Check config file exists
  if (!SPIFFS.exists(CONFIG_FILENAME)) {
    Serial.println("HALTING!!! Config file not present!");
    while(true);
  }

  Serial.println("Reading configuration");
  File fin = SPIFFS.open(CONFIG_FILENAME);
  deserializeJson(config, fin);
  Serial.print("Node ID: ");
  Serial.println(config["id"].as<int>());

  connectAWS();

  publish_backlog();

  Serial.println("Setting up!\n");

  if (! bme.begin(0x77, &Wire)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  SPI.begin(); // For SPI
  if(!lightning.beginSPI(LIGHTN_CS) ) { 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  lightning.maskDisturber((bool)config["mask_disturber"]);
  if (config["indoor"])
    lightning.setIndoorOutdoor(INDOOR);
  else
    lightning.setIndoorOutdoor(OUTDOOR);
  lightning.setNoiseLevel(config["noise_level"]);
  lightning.spikeRejection(config["spike_rejection"]);
  lightning.lightningThreshold(config["lightning_threshold"]);
  lightning.watchdogThreshold(config["watchdog_threshold"]);
  lightning.clearStatistics(true);
  sawLightning = false;
  attachInterrupt(LIGHTN_INT, LIGHTN_ISR, HIGH);
}

void loop() {
  if (digitalRead(LIGHTN_INT)) {
    sawLightning = false;
    Serial.println("Lightning interrupt recieved");
    delay(2);
    int intVal = lightning.readInterruptReg();

    if (intVal == NOISE_INT) {
      Serial.println("Noise.");
    } else if (intVal == DISTURBER_INT) {
      Serial.println("Disturber.");
    } else if (intVal == LIGHTNING_INT) {
      Serial.println("Lightning Strike Detected!");
      // Lightning! Now how far away is it? Distance estimation takes into
      // account previously seen events.
      byte distance = lightning.distanceToStorm();
      Serial.print("Approximately: ");
      Serial.print(distance);
      Serial.println("km away!");

      // "Lightning Energy" and I do place into quotes intentionally, is a pure
      // number that does not have any physical meaning.
      long lightEnergy = lightning.lightningEnergy();
      Serial.print("Lightning Energy: ");
      Serial.println(lightEnergy);
    } else {
      Serial.print("Interrupt value: ");
      Serial.println(intVal);
    }
  } else {
    take_reading();
  }
  //Serial.println("Looped\n");
  
  //print_hw_debug();
  delay(5000);
}