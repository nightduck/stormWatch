#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "SPIFFS.h"
#include "Adafruit_BME280.h"
#include "SparkFun_AS3935.h"

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

#define CONFIG_FILENAME "/config"
#define WEATHER_FILENAME "/weather_readings"
#define LIGHTNING_FILENAME "/lightning_events"

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

typedef struct config {
  int node_num;
} config_t;

config_t cfg;
bool online;

Adafruit_BME280 bme;
SparkFun_AS3935 lightning;
bool sawLightning;
bool factoryReset;

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


  // config_t temp;
  // esp_partition_read(data_partition, CONFIG_ADDR, &temp, sizeof(config_t));
  // Serial.print("Node ID: ");
  // Serial.println(temp.node_num);
  // Serial.print("Weather address: ");
  // Serial.println((int)temp.weather_ptr);
  // Serial.print("Lightning address: ");
  // Serial.println((int)temp.lightning_ptr);
  // Serial.println();
}

void factory_reset() {
  File config = SPIFFS.open("/config", FILE_WRITE);
  Serial.println("Creating config file");
  config_t cfg;
  cfg.node_num = read_counter();
  config.write((uint8_t*)&cfg, sizeof(cfg));
  config.close();
}

bool record_weather_reading(const weather_reading_t *reading) {
  // If in online mode, publish to cloud
  if (online) {
    online = false; // TODO: Implement online features, for now: fail
  }
  Serial.println("Recording weather reading");
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

    return true;
  }

  return false; // IDK how you could possibly reach here
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
    // TODO: Replace this with WiFi upload
    Serial.print("Temperature = ");
    Serial.print(reading.temp);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(reading.pressure);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(reading.humidity);
    Serial.println(" %");

    Serial.print("Rainfall = ");
    Serial.print(reading.rainfall_mm);
    Serial.println(" mm");

    Serial.print("Wind direction = ");
    Serial.println(reading.wind_direction);

    Serial.print("Humidity = ");
    Serial.print(reading.wind_speed);
    Serial.println(" kmh");
    Serial.println();
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

  int count = read_counter();
  reset_counter();
  Serial.print("Counter = ");
  Serial.println(count);
  
  Serial.print("Temperature = ");
  Serial.print(reading.temp);
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(reading.pressure);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(reading.humidity);
  Serial.println(" %");

  if (!record_weather_reading(&reading)) {
    Serial.println("FAILED TO SAVE DATA. ABORTING");
    print_hw_debug();
    while(true);
  };
}

void IRAM_ATTR FACTORY_RESET_ISR() {
  factoryReset = true;
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
    Serial.print("Size of filesystem: ");
    Serial.println(SPIFFS.totalBytes());
    Serial.print("Used: ");
    Serial.println(SPIFFS.usedBytes());
  }

  // Open the config file. If none exist, do initial configuration
  if (!SPIFFS.exists("/config")) {
    factory_reset();
  }

  Serial.println("Setting up!\n");

  File config = SPIFFS.open("/config", FILE_READ);
  config_t cfg;
  config.read((uint8_t*)&cfg, sizeof(cfg));
  Serial.print("Node number: ");
  Serial.println(cfg.node_num);
  config.close();

  publish_backlog();

  if (! bme.begin(0x77, &Wire)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  SPI.begin(); // For SPI
  if(!lightning.beginSPI(LIGHTN_CS) ) { 
    Serial.println ("Lightning Detector did not start up, freezing!"); 
    while(1); 
  }
  lightning.maskDisturber(false);
  lightning.setIndoorOutdoor(INDOOR);
  lightning.setNoiseLevel(5);
  lightning.spikeRejection(1);
  lightning.lightningThreshold(1);
  lightning.watchdogThreshold(3);
  lightning.clearStatistics(true);
  sawLightning = false;
  attachInterrupt(LIGHTN_INT, LIGHTN_ISR, HIGH);

  // factoryReset = false;
  // attachInterrupt(FACTORY_RESET, FACTORY_RESET_ISR, RISING);
}

void loop() {
  if (factoryReset) {
    factory_reset();

    // while(digitalRead(FACTORY_RESET));
    // delay(100);
    // ESP.restart();
  } else if (digitalRead(LIGHTN_INT)) {
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
    //take_reading();
  }
  //Serial.println("Looped\n");
  
  //print_hw_debug();
  delay(100);
}