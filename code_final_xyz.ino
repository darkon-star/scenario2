#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_AHTX0.h>
#include <math.h>

Adafruit_AHTX0 aht;

float X, Y, Z;
float magnitude;
float previousMagnitude = 0;
float movementChange;

float movementSum = 0;
int sampleCount = 0;
float avgMovement = 0;

float breathingThreshold = 0.60;

float temperatureC;
float warningTemp = 37.5;
float alertTemp = 38.0;

unsigned long startTime;
unsigned long lastCheckTime = 0;

unsigned long lowMovementStart = 0;
unsigned long warningTime = 10000;
unsigned long alertTime = 20000;

unsigned long silenceUntil = 0;
unsigned long silenceDuration = 10000;

void setAllPixels(uint32_t color) {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, color);
  }
}

void alertSound() {
  for (int i = 0; i < 3; i++) {
    CircuitPlayground.playTone(1760, 100);
    delay(80);
  }
}

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();

  CircuitPlayground.setAccelRange(LIS3DH_RANGE_2_G);

  if (!aht.begin()) {
    Serial.println("Could not find AHT20. Check wiring.");
    while (1) {
      delay(10);
    }
  }

  X = CircuitPlayground.motionX();
  Y = CircuitPlayground.motionY();
  Z = CircuitPlayground.motionZ();
  previousMagnitude = sqrt(X * X + Y * Y + Z * Z);

  startTime = millis();

  Serial.println("Time_s,Temperature_C,Avg_Movement,Temperature_Status,Movement_Status,Final_Status");
}

void loop() {
  unsigned long now = millis();

  if (CircuitPlayground.rightButton()) {
    lowMovementStart = 0;
    silenceUntil = now + silenceDuration;
    movementSum = 0;
    sampleCount = 0;

    CircuitPlayground.clearPixels();
    setAllPixels(0x0000FF);

    delay(500);
  }

  X = CircuitPlayground.motionX();
  Y = CircuitPlayground.motionY();
  Z = CircuitPlayground.motionZ();

  magnitude = sqrt(X * X + Y * Y + Z * Z);
  movementChange = fabs(magnitude - previousMagnitude);
  previousMagnitude = magnitude;

  movementSum += movementChange;
  sampleCount++;

  if (now - lastCheckTime >= 1000) {
    avgMovement = movementSum / sampleCount;

    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    temperatureC = temp.temperature;

    String tempStatus = "TEMP_NORMAL";
    String movementStatus = "MOVEMENT_NORMAL";
    String finalStatus = "NORMAL";

    if (temperatureC >= alertTemp) {
      tempStatus = "TEMP_ALERT";
    } else if (temperatureC >= warningTemp) {
      tempStatus = "TEMP_WARNING";
    } else {
      tempStatus = "TEMP_NORMAL";
    }

    if (avgMovement < breathingThreshold) {
      if (lowMovementStart == 0) {
        lowMovementStart = now;
      }

      unsigned long lowDuration = now - lowMovementStart;

      if (lowDuration >= alertTime) {
        movementStatus = "MOVEMENT_ALERT";
      } else if (lowDuration >= warningTime) {
        movementStatus = "MOVEMENT_WARNING";
      } else {
        movementStatus = "MOVEMENT_NORMAL";
      }

    } else {
      lowMovementStart = 0;
      movementStatus = "MOVEMENT_NORMAL";
    }

    if (now < silenceUntil) {
      finalStatus = "SILENCED";
      CircuitPlayground.clearPixels();
      setAllPixels(0x0000FF);

    } else if (tempStatus == "TEMP_ALERT" || movementStatus == "MOVEMENT_ALERT") {
      finalStatus = "ALERT";
      CircuitPlayground.clearPixels();
      setAllPixels(0xFF0000);
      alertSound();

    } else if (tempStatus == "TEMP_WARNING" || movementStatus == "MOVEMENT_WARNING") {
      finalStatus = "WARNING";
      CircuitPlayground.clearPixels();
      setAllPixels(0xFFFF00);

    } else {
      finalStatus = "NORMAL";
      CircuitPlayground.clearPixels();
      setAllPixels(0x00FF00);
    }

    float time_s = (now - startTime) / 1000.0;

    Serial.print(time_s);
    Serial.print(",");
    Serial.print(temperatureC);
    Serial.print(",");
    Serial.print(avgMovement);
    Serial.print(",");
    Serial.print(tempStatus);
    Serial.print(",");
    Serial.print(movementStatus);
    Serial.print(",");
    Serial.println(finalStatus);

    movementSum = 0;
    sampleCount = 0;
    lastCheckTime = now;
  }

  delay(50);
}