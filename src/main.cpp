#include <Arduino.h>
#define COUNTER_0 17
#define COUNTER_1 21
#define COUNTER_2 12
#define COUNTER_3 27
#define COUNTER_4 33
#define COUNTER_5 15
#define COUNTER_6 32
#define COUNTER_7 14
#define COUNTER_RST 16
#define LIGHTN_INT 4
#define ANEM_INT 36 
#define LED 13

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
  Serial.begin(9600);
  // put your setup code here, to run once:
}

void loop() {
  int count = read_counter();
  reset_counter();

  Serial.println("Looped");
  Serial.println(count);
  delay(1000);
  // put your main code here, to run repeatedly:
}