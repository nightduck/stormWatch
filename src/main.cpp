#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "EEPROM.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_partition.h"
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
#define LIGHTN_CS 21
#define BATT 35
#define LED 13
#define SEALEVELPRESSURE_HPA 1013.25

#define INDOOR 0x12 
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01

const esp_partition_t* data_partition;

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
} lightn_reading_t;

typedef struct config {
  size_t weather_ptr;
  size_t lightning_ptr;
  int node_num;
} config_t;

config_t cfg;
bool online;

Adafruit_BME280 bme;
SparkFun_AS3935 lightning;
bool sawLightning;

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

  // Serial.print("Writing reading to address ");
  // Serial.println(weather_reading_ptr);
  // EEPROM.put(weather_reading_ptr, reading);
  // weather_reading_ptr += sizeof(weather_reading_t);
  // EEPROM.write(0, weather_reading_ptr);
  // EEPROM.commit();
  // Serial.println("Successful");
}

bool record_weather_reading(const weather_reading_t *reading) {
  // If in online mode, publish to cloud
  if (online) {
    online = false; // TODO: Implement online features, for now: fail
    return true;
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  if (!online) {
    // Record a weather reading by adding data to bottom of data partition
    if (cfg.weather_ptr + sizeof(weather_reading_t) > cfg.lightning_ptr) return false;

    if (esp_partition_write(data_partition, cfg.weather_ptr, reading, sizeof(reading)) != ESP_OK)
      return false;

    cfg.weather_ptr += sizeof(reading);
    return (esp_partition_write(data_partition, 0, &cfg, sizeof(cfg)) == ESP_OK);
  }

  return false; // IDK how you could possibly reach here
}

bool record_lightning_event(const lightn_reading_t *event) {
  // If in online mode, publish to cloud
  if (online) {
    online = false; // TODO: Implement online features, for now: fail
    return true;
  }

  // If in offline mode (or if most recent reading failed to publish),
  // save to flash
  if (!online) {
      // Record a lightning event by adding data to bottom of data partition
      if (cfg.lightning_ptr - sizeof(event) < cfg.lightning_ptr) return false;

      cfg.lightning_ptr -= sizeof(event);
      if (esp_partition_write(data_partition, cfg.lightning_ptr, event, sizeof(event)) != ESP_OK)
        return false;

      return (esp_partition_write(data_partition, 0, &cfg, sizeof(cfg)) == ESP_OK);
  }

  return false; // IDK how you could possibly reach here
}

void factory_reset() {
  Serial.println("Factory reset");
  cfg.node_num = 4;
  cfg.lightning_ptr = data_partition->size;
  cfg.weather_ptr = sizeof(config_t);
  esp_partition_write(data_partition, 0, &cfg, sizeof(config_t));

  Serial.print("Node ID: ");
  Serial.println(cfg.node_num);
  Serial.print("Weather address: ");
  Serial.println((int)cfg.lightning_ptr);
  Serial.print("Node ID: ");
  Serial.println((int)cfg.weather_ptr);
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
  Serial.println();
}

void IRAM_ATTR LIGHTN_ISR() {
  sawLightning = true;
}

void setup() {
  Serial.begin(9600);
  //EEPROM.begin(EEPROM_SIZE);
  delay(500);
  
  // weather_reading_ptr = EEPROM.read(0);
  // lightn_reading_ptr = EEPROM.read(1);

  // if(weather_reading_ptr > 2) {
  //   Serial.println("Previous readings exist");
  // }
  //   Serial.print("weather_reading_ptr: ");
  //   Serial.println(weather_reading_ptr);
  //   Serial.print("sizeof(weather_reading_ptr): ");
  //   Serial.println(sizeof(weather_reading_t));
  //   Serial.print("Number of readings: ");
  //   Serial.println((weather_reading_ptr - 2)/sizeof(weather_reading_t));

  // for(; weather_reading_ptr > 2; weather_reading_ptr -= sizeof(weather_reading_t)) {
  //   weather_reading_t reading;
  //   EEPROM.get(weather_reading_ptr, reading);

  //   Serial.println("Old reading: ");
  //   Serial.print("Temperature = ");
  //   Serial.print(reading.temp);
  //   Serial.println(" *C");

  //   Serial.print("Pressure = ");
  //   Serial.print(reading.pressure);
  //   Serial.println(" hPa");

  //   Serial.print("Humidity = ");
  //   Serial.print(reading.humidity);
  //   Serial.println(" %");
  //   delay(500);
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
  online = true;

  data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "app_data");

  // If factory reset button held down
  if (false) {
    factory_reset();
    while(false)
      delay(100);
    ESP.restart();
  } else {
    esp_partition_read(data_partition, 0, &cfg, sizeof(config_t));

    Serial.print("Node ID: ");
    Serial.println(cfg.node_num);
    Serial.print("Weather address: ");
    Serial.println((int)cfg.weather_ptr);
    Serial.print("Node ID: ");
    Serial.println((int)cfg.lightning_ptr);
  }

  while(true);

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
  lightning.maskDisturber(false);
  lightning.setIndoorOutdoor(INDOOR);
  lightning.setNoiseLevel(5);
  lightning.spikeRejection(1);
  lightning.lightningThreshold(1);
  sawLightning = false;
  attachInterrupt(LIGHTN_INT, LIGHTN_ISR, HIGH);
}

void loop() {
  //take_reading();

  if (sawLightning) {
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
  //Serial.println("Looped\n");
  //print_hw_debug();
  delay(100);
}