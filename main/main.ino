#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "esp_pm.h"
#include "SPIFFS.h"
#include "Adafruit_BME280.h"
#include "SparkFun_AS3935.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

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
#define CONFIG_TOPIC        "config/" + NODENAME
#define WEATHER_PUB_TOPIC   "sensor/" + NODENAME + "/weather"
#define LIGHTNING_PUB_TOPIC "sensor/" + NODENAME + "/lightning"

#define MS_PER_MIN    60000L
#define EMA_FACTOR    0.05

String NODENAME;

int WAKING_PERIOD;  // Number of seconds between weather readings
int RETRIES = 4;   // Number of retries when connecting MQTT client

bool fullNode;

typedef struct weather_reading {
  float temp;
  float pressure;
  float humidity;
  int wind_direction;
  float wind_speed;
  float rainfall_mm;
  time_t timestamp;
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

time_t lastNTP;
ulong lastSyncMillis;
long lastDrift;       // Milliseconds lost per observed minute between last two syncs
long driftEMA;        // Running EMA of lastDrift

char AWS_CERT_CA[1280];
char AWS_CERT_CRT[1280];
char AWS_CERT_PRIVATE[1792];

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

// Returns seconds since epoch
ulong now() {
  ulong timeSinceLastSync = millis() - lastSyncMillis;
  ulong drift = timeSinceLastSync / MS_PER_MIN * driftEMA;

  return (timeSinceLastSync + drift) / 1000 + lastNTP;
}

void read_config() {
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

  WAKING_PERIOD = config["waking_period"].as<int>();
  driftEMA = config["drift"].as<long>();
}

void downloadNewConfigWeb() {
  HTTPClient ec2_client;
  String host = config["ec2_endpoint"];
  String path = "/config_" + NODENAME + ".json";

  Serial.print("Connecting to: ");
  Serial.println(host + ":5000" + path);

  ec2_client.begin(host, 5000, path);

  if (ec2_client.GET() > 0) {
    String payload = ec2_client.getString();
    Serial.println("Got config");

    deserializeJson(config, payload);

    File cfg_file = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
    if (cfg_file.write((uint8_t*)payload.c_str(), payload.length()) < payload.length()) {
      Serial.println("!!! New config not correctly saved");
    }
  } else {
    Serial.println("Didn't get a response");
  }

  return;
  
}

void downloadNewConfig(String &topic, String &payload) {
  Serial.println("Got message!");
  StaticJsonDocument<256> cfg;
  deserializeJson(cfg, payload);

  Serial.println(payload);

  if(cfg.containsKey("mask_disturbers") && cfg["mask_disturbers"].is<bool>()) {
    config["mask_disturbers"] = cfg["mask_disturbers"];
  }
  if(cfg.containsKey("indoor") && cfg["indoor"].is<bool>()) {
    config["indoor"] = cfg["indoor"];
  }
  if(cfg.containsKey("noise_level") && cfg["noise_level"].is<int>()) {
    config["noise_level"] = cfg["noise_level"];
  }
  if(cfg.containsKey("spike_rejection") && cfg["spike_rejection"].is<int>()) {
    config["spike_rejection"] = cfg["spike_rejection"];
  }
  if(cfg.containsKey("lightning_threshold") && cfg["lightning_threshold"].is<int>()) {
    config["lightning_threshold"] = cfg["lightning_threshold"];
  }
  if(cfg.containsKey("watchdog_threshold") && cfg["watchdog_threshold"].is<int>()) {
    config["watchdog_threshold"] = cfg["watchdog_threshold"];
  }
  if(cfg.containsKey("waking_period") && cfg["waking_period"].is<int>()) {
    config["waking_period"] = cfg["waking_period"];
  }
  if(cfg.containsKey("drift") && cfg["drift"].is<long>()) {
    config["drift"] = cfg["drift"];
  }

  // TODO: Wifi check with fallback??
  if(cfg.containsKey("wifi_ssid") && cfg["wifi_ssid"].is<String>()) {
    config["wifi_ssid"] = cfg["wifi_ssid"];
  }
  if(cfg.containsKey("wifi_password") && cfg["wifi_password"].is<String>()) {
    config["wifi_password"] = cfg["wifi_password"];
  }

  read_config();

  if(cfg.containsKey("online") && !cfg["online"].as<bool>()) {
    // TODO: Turn off wifi until next power cycle or hard reset
  }

  serializeJson(config, payload);
  File cfg_file = SPIFFS.open(CONFIG_FILENAME, FILE_WRITE);
  if (cfg_file.write((uint8_t*)payload.c_str(), payload.length()) < payload.length()) {
    Serial.println("!!! New config not correctly saved");
  }
}

void syncTime(bool init=false) {
  time_t currentNTP;
  // Connect to NTP server and set time
  configTime(0, 0, "time.google.com", "time-c-wwv.nist.gov", "pool.ntp.org");
  if (init)
    delay(500);
  ulong currentMillis = millis();
  if (!time(&currentNTP)) {
    Serial.println("HALTING!!! Can't get NTP time");
    while(true);
  }

  if (init) {
    driftEMA = config["drift"].as<long>();
    lastDrift = driftEMA;
  } else {
    long diffNTP_ms = (currentNTP - lastNTP) * 1000;
    long diffMillis_ms = currentMillis - lastSyncMillis;

    long drift = (long)(((long long)diffNTP_ms * MS_PER_MIN) / diffMillis_ms) - MS_PER_MIN;

    driftEMA = driftEMA * (1 - EMA_FACTOR) + drift * EMA_FACTOR;
    lastDrift = drift;
  }

  lastNTP = currentNTP;
  lastSyncMillis = currentMillis;
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

bool connectMQTT() {
  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(config["aws_iot_endpoint"].as<const char*>(), 8883, net);

  // Create a message handler
  client.onMessage(downloadNewConfig);

  Serial.print("Connecting to AWS IOT");

  for(int i=0; i < RETRIES && !client.connect(config["thingname"].as<const char*>()); i++) {
    Serial.print(".");
    delay(1000);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    Serial.println("Falling back to offline mode!!");
    online = false;
    return false;
  } else {
    Serial.println("AWS IoT Connected!");
    online = true;
  }

  // Subscribe to a topic
  client.subscribe(CONFIG_TOPIC, 1);

  return true;
}

void loadCerts() {
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
}

bool connectAWS()
{  
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  return connectMQTT();
}

bool reconnectToWifi() {
  WiFi.reconnect();
  while (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_NO_SHIELD){
    delay(500);
    Serial.print(".");
  }
  if(WiFi.isConnected()) {
    connectAWS();
    syncTime();
  } else {
    Serial.println("Wifi not present. Falling back to offline mode.");
    Serial.print("Wifi status: ");
    switch(WiFi.status()) {
      case WL_IDLE_STATUS:
        Serial.println("WL_IDLE_STATUS");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("WL_NO_SSID_AVAIL");
        break;
      case WL_SCAN_COMPLETED:
        Serial.println("WL_SCAN_COMPLETED");
        break;
      case WL_CONNECTED:
        Serial.println("WL_CONNECTED");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("WL_CONNECT_FAILED");
        break;
      case WL_CONNECTION_LOST:
        Serial.println("WL_CONNECTION_LOST");
        break;
      case WL_DISCONNECTED:
        Serial.println("WL_DISCONNECTED");
        break;
      default:
        Serial.println(WiFi.status());
    }
    online = false;
  }

  return online;
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
  Serial.println();
}

bool record_weather_reading(const weather_reading_t *reading) {
  // If in online mode, publish to cloud
  if (online) {
    StaticJsonDocument<256> jsonDoc;
    JsonObject stateObj = jsonDoc.createNestedObject("state");
    JsonObject reportedObj = stateObj.createNestedObject("reported");
    if (fullNode) {   // Only report sensor data when you have sensors
      reportedObj["temp"] = reading->temp;
      reportedObj["pressure"] = reading->pressure;
      reportedObj["humidity"] = reading->humidity;
      reportedObj["wind_direction"] = reading->wind_direction;
      reportedObj["wind_speed"] = reading->wind_speed;
      reportedObj["rainfall_mm"] = reading->rainfall_mm;
    }
    reportedObj["battery"] = readBattery();
    reportedObj["timestamp"] = reading->timestamp;
    reportedObj["drift"] = driftEMA;
    reportedObj["node"] = NODENAME;
    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);
    Serial.println(jsonBuffer);

    if(!client.publish(WEATHER_PUB_TOPIC, jsonBuffer, false, 1)) {
      Serial.println("Failed to publish");
    } else
    {
      Serial.println("Published reading");
      return true;
    }
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash. If lightning-only mode, just skip. We have no need
  // for node stats that aren't real-time
  if (fullNode) {
    File weather = SPIFFS.open(WEATHER_FILENAME, FILE_APPEND);
    if (!weather) {
      return false;
    }
    if (weather.write((uint8_t*)reading, sizeof(weather_reading_t)) < sizeof(weather_reading_t)) {
      return false;
    }
  }

  Serial.println("Saved reading to flash");
  return true;
}

bool record_lightning_event(const lightn_event_t *event) {
  // If in online mode, publish to cloud
  if (online) {
    StaticJsonDocument<256> jsonDoc;
    JsonObject stateObj = jsonDoc.createNestedObject("state");
    JsonObject reportedObj = stateObj.createNestedObject("reported");
    reportedObj["power"] = event->power;
    reportedObj["timestamp"] = event->timestamp;
    reportedObj["node"] = NODENAME;
    char jsonBuffer[256];
    serializeJson(jsonDoc, jsonBuffer);
    Serial.println(jsonBuffer);

    if(!client.publish(LIGHTNING_PUB_TOPIC, jsonBuffer, false, 1)) {
      Serial.println("Failed to publish");
    } else
    {
      Serial.println("Published reading");
      return true;
    }
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  File lightning = SPIFFS.open(WEATHER_FILENAME, FILE_APPEND);
  if (!lightning) {
    return false;
  }
  if (lightning.write((uint8_t*)event, sizeof(lightn_event_t)) < sizeof(lightn_event_t)) {
    return false;
  }

  return true;
}

bool publish_backlog() {
  Serial.println("Inspecting offline backlog");
  SPIFFS.rename(WEATHER_FILENAME, WEATHER_FILENAME_TEMP);
  SPIFFS.rename(LIGHTNING_FILENAME, LIGHTNING_FILENAME_TEMP);
  File weather = SPIFFS.open(WEATHER_FILENAME_TEMP, FILE_READ);
  File lightning = SPIFFS.open(LIGHTNING_FILENAME_TEMP, FILE_READ);
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
    record_lightning_event(&event);
  }

  SPIFFS.remove(WEATHER_FILENAME_TEMP);
  SPIFFS.remove(LIGHTNING_FILENAME_TEMP);
  return true;
}

void take_reading() {
  weather_reading_t reading;
  if (fullNode) {     // If lightning-only nodes, this method is for reporting node stats only
    reading.temp = bme.readTemperature();
    reading.pressure = bme.readPressure() / 100.0F;
    reading.humidity = bme.readHumidity();
  }
  reading.timestamp = now();
  // TODO: More sensors here

  if (!record_weather_reading(&reading)) {
    Serial.println("FAILED TO SAVE DATA. ABORTING");
    print_hw_debug();
    while(true);
  };
}

void IRAM_ATTR LIGHTN_ISR() {
  sawLightning = true;
}

void enterSleep() {
  esp_wifi_stop();
  digitalWrite(LED, 0);
  esp_light_sleep_start();    // Spend 5 minutes here
  digitalWrite(LED, 1);
  if (esp_wifi_start() != ESP_OK) {
    Serial.println("Error restarting wifi");
  }
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

  Serial.print("Node Name: ");
  NODENAME = config["thingname"].as<String>();
  Serial.println(NODENAME);

  char jsonBuffer[1024];
  serializeJson(config, jsonBuffer);
  Serial.println(jsonBuffer);

  fullNode = config["full_node"].as<bool>();

  connectToWiFi();

  syncTime(true);

  loadCerts();
  connectAWS();

  publish_backlog();

  client.disconnect();

  Serial.println("Setting up!\n");

  if (fullNode) {
    if (! bme.begin(0x77, &Wire)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
    }
  }

  SPI.begin(); // For SPI
  if(!lightning.beginSPI(LIGHTN_CS) ) { 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }

  read_config();    // Set all the hardware settings based on the loaded configuration (noise threshhold, polling rate, etc)

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

  esp_sleep_enable_timer_wakeup(WAKING_PERIOD * 1000000);   // Take first reading 1sec after setup
  esp_sleep_enable_gpio_wakeup();
}

void loop() {
  enterSleep();   // Turn off LED and WiFi, light sleep for 5 minutes, then turn on LED and WiFi

  time_t timestamp = now();   // Get timestamp immediately, so the delay in connecting to wirless doesn't corrupt it
  reconnectToWifi();

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

          lightn_event_t event;
          event.power = lightning.lightningEnergy();
          event.timestamp = timestamp;

          if (!record_lightning_event(&event)) {
            Serial.println("FAILED TO SAVE DATA. ABORTING");
            print_hw_debug();
            while(true);
          };

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
      esp_sleep_enable_timer_wakeup(WAKING_PERIOD * 1000000); 
      break;
    default:
      Serial.println("Default");
  }

  if (online) {
    downloadNewConfigWeb();
    client.loop();
    client.disconnect();
  }

  delay(20);
  
  //print_hw_debug();
}