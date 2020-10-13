#include <Arduino.h>
#include "SPI.h"
#include "Wire.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
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
  int count = read_counter();
  reset_counter();
  Serial.print("Counter = ");
  Serial.println(count);
  
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");
}

void IRAM_ATTR LIGHTN_ISR() {
  sawLightning = true;
}

void setup() {
  Serial.begin(9600);
  delay(500);

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
  lightning.setNoiseLevel(1);
  lightning.spikeRejection(1);
  lightning.lightningThreshold(1);
  sawLightning = false;
  attachInterrupt(LIGHTN_INT, LIGHTN_ISR, HIGH);
}

void loop() {
  take_reading();

  for(int reg = 0; reg < 9; reg++) {
    Serial.print("Register ");
    Serial.print(reg);
    Serial.print(": ");
    Serial.println(lightning._readRegister(reg));
  }

  if (digitalRead(LIGHTN_INT) == 1) {
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
  Serial.println("Looped\n");
  delay(1000);
}