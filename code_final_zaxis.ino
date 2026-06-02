#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_AHTX0.h>
#include <math.h>

Adafruit_AHTX0 aht;

float X, Y, Z;

float zMin = 9999;
float zMax = -9999;
float zRange = 0;

float breathingRangeThreshold = 0.55;

float temperatureC;
float warningTemp = 37.5;
float alertTemp = 38.0;

unsigned long startTime;
unsigned long windowStartTime = 0;
unsigned long windowDuration = 3000;

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

  startTime = millis();
  windowStartTime = millis();

  Serial.println("Time_s,Temperature_C,Z_Range,Temperature_Status,Movement_Status,Final_Status");
}

void loop() {
  unsigned long now = millis();

  if (CircuitPlayground.rightButton()) {
    lowMovementStart = 0;
    silenceUntil = now + silenceDuration;

    CircuitPlayground.clearPixels();
    setAllPixels(0x0000FF);

    delay(500);
  }

  X = CircuitPlayground.motionX();
  Y = CircuitPlayground.motionY();
  Z = CircuitPlayground.motionZ();

  if (Z < zMin) {
    zMin = Z;
  }

  if (Z > zMax) {
    zMax = Z;
  }

  if (now - windowStartTime >= windowDuration) {
    zRange = zMax - zMin;

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

    if (zRange < breathingRangeThreshold) {
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
    Serial.print(zRange);
    Serial.print(",");
    Serial.print(tempStatus);
    Serial.print(",");
    Serial.print(movementStatus);
    Serial.print(",");
    Serial.println(finalStatus);

    zMin = 9999;
    zMax = -9999;
    windowStartTime = now;
  }

  delay(50);
}