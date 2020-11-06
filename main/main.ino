#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_bt.h"
#include "esp_pm.h"
#include "SPIFFS.h"
#include "Adafruit_BME280.h"
#include "SparkFun_AS3935.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

#define COUNTER_0 GPIO_NUM_25
#define COUNTER_1 GPIO_NUM_26
#define COUNTER_2 GPIO_NUM_12
#define COUNTER_3 GPIO_NUM_27
#define COUNTER_4 GPIO_NUM_33
#define COUNTER_5 GPIO_NUM_15
#define COUNTER_6 GPIO_NUM_32
#define COUNTER_7 GPIO_NUM_14
#define COUNTER_RST GPIO_NUM_34
#define LIGHTN_INT GPIO_NUM_4
#define ANEM_INT GPIO_NUM_36
#define WEATHER_VANE GPIO_NUM_39 
#define LIGHTN_CS GPIO_NUM_21
#define BATT GPIO_NUM_35
#define LED GPIO_NUM_13
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
#define WEATHER_FILENAME_TEMP "/weather_readings.tmp"
#define LIGHTNING_FILENAME_TEMP "/lightning_events.tmp"

// The MQTT topics that this device should publish/subscribe
#define CONFIG_TOPIC_LEADER "config/"
#define WEATHER_PUB_TOPIC   "node01/weather"
#define LIGHTNING_PUB_TOPIC   "node01/lightning"

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

void downloadNewConfig(String &topic, String &payload) {
  Serial.println("Got message!");
  StaticJsonDocument<256> jsonDoc;
  deserializeJson(jsonDoc, payload);
  char jsonBuffer[256];
  serializeJson(jsonDoc["state"]["reported"], jsonBuffer);
  Serial.println(jsonBuffer);
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

void connectMQTT() {
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(config["aws_iot_endpoint"].as<const char*>(), 8883, net);

  // Create a message handler
  client.onMessage(downloadNewConfig);

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
  client.subscribe("config/node01", 1);
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
  Serial.println(AWS_CERT_CA);
  delay(500);

  temp = "";
  while (certCRT.available()) {
    temp += certCRT.readString();
  }
  strcpy(AWS_CERT_CRT, temp.c_str());
  Serial.println(AWS_CERT_CRT);
  delay(500);

  temp = "";
  while (certPrivate.available()) {
    temp += certPrivate.readString();
  }
  strcpy(AWS_CERT_PRIVATE, temp.c_str());
  Serial.println(AWS_CERT_PRIVATE);
  delay(500);

  certCA.close();
  certCRT.close();
  certPrivate.close();

  connectToWiFi();
  
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  connectMQTT();

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
  Serial.print("Heap size: ");
  Serial.println(ESP.getHeapSize());
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
    reportedObj["battery"] = readBattery();
    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);
    Serial.println(jsonBuffer);

    if(!client.publish(WEATHER_PUB_TOPIC, jsonBuffer, false, 1)) {
      Serial.println("Failed to publish");
      //online = false;
    } else
    {
      Serial.println("Published reading");
      return true;
    }
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  File weather = SPIFFS.open(WEATHER_FILENAME, FILE_APPEND);
  if (!weather) {
    return false;
  }
  if (weather.write((uint8_t*)reading, sizeof(weather_reading_t)) < sizeof(weather_reading_t)) {
    return false;
  }

  Serial.println("Saved reading to flash");
  return true;
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
  SPIFFS.rename(WEATHER_FILENAME, WEATHER_FILENAME_TEMP);
  SPIFFS.rename(LIGHTNING_FILENAME, LIGHTNING_FILENAME_TEMP);
  File weather = SPIFFS.open(WEATHER_FILENAME_TEMP, FILE_READ);
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
    // TODO: Bug where if online fails, then recordings are doubled
  };

  lightn_event_t event;
  while(lightning.read((uint8_t*)&event, sizeof(lightn_event_t)) != 0) {
    // TODO: Replace this with WiFi upload
    Serial.print("Lightning energy = ");
    Serial.print(event.power);

    Serial.print("Timestamp = ");
    Serial.print(event.timestamp);
  }

  SPIFFS.remove(WEATHER_FILENAME_TEMP);
  SPIFFS.remove(LIGHTNING_FILENAME_TEMP);
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
  digitalWrite(LED, 1);

  Serial.begin(115200);
  delay(500);
  Serial.println("Serial initialized");

  // Serial.println("Awaken get ready...");
  // for(int i = 0; i < 10; i++) {
  //   Serial.println(i);
  //   delay(1000);
  // }

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
  lightning.maskDisturber((bool)config["mask_disturbers"]);
  if (config["indoor"]) {
    lightning.setIndoorOutdoor(INDOOR);
  } else {
    lightning.setIndoorOutdoor(OUTDOOR);
  }
  lightning.setNoiseLevel(config["noise_level"]);
  lightning.spikeRejection(config["spike_rejection"]);
  lightning.lightningThreshold(config["lightning_threshold"]);
  lightning.watchdogThreshold(config["watchdog_threshold"]);
  lightning.clearStatistics(true);

  char jsonBuffer[1024];
  serializeJson(config, jsonBuffer);
  Serial.println(jsonBuffer);

  sawLightning = false;
  attachInterrupt(LIGHTN_INT, LIGHTN_ISR, HIGH);
  gpio_wakeup_enable(LIGHTN_INT, GPIO_INTR_HIGH_LEVEL);

  // Turn off bluetooth
  esp_bt_controller_disable();

#ifndef CONFIG_PM_ENABLE
#error "CONFIG_PM_ENABLE missing"
#endif
    // esp_pm_config_esp32_t pm_config;
    // pm_config.max_freq_mhz = 80;
    // pm_config.min_freq_mhz = 13;
    // pm_config.light_sleep_enable = false;
    // ESP_ERROR_CHECK( esp_pm_configure(&pm_config) );

  esp_sleep_enable_timer_wakeup(300000000);
  esp_sleep_enable_gpio_wakeup();
}

void loop() {
  digitalWrite(LED, 1);
  if (online) {
    client.loop();
    if (!client.connected()) {
      Serial.println("Reconnecting...");
      if (WiFi.isConnected()) {
        Serial.println("Wifi still connected");
      } else {
        Serial.println("Wifi not connected");
      }
      connectMQTT();
      publish_backlog();
    }
  }

  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_GPIO:
      Serial.println("Wokeup because lightning");
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
      }
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wokeup because timer");
      take_reading();
      break;
    default:
      Serial.println("Default");
  }
  
  print_hw_debug();

  delay(20);
  digitalWrite(LED, 0);
  esp_light_sleep_start();
}