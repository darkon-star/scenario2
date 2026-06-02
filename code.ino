#include <Adafruit_CircuitPlayground.h>
#include <math.h>

float X, Y, Z;
float magnitude;
float previousMagnitude = 0;
float movementChange;

float movementSum = 0;
int sampleCount = 0;
float avgMovement = 0;

float breathingThreshold = 4.5; // data resaerch

unsigned long startTime;
unsigned long lastCheckTime = 0;
unsigned long lowMovementStart = 0;

void setAllPixels(uint32_t color) {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, color);
  }
}

void setup() {
  Serial.begin(115200);
  CircuitPlayground.begin();

  CircuitPlayground.setAccelRange(LIS3DH_RANGE_2_G);

  startTime = millis();

  Serial.println("time_s,avg_movement_1s,state");
}

void loop() {
  X = CircuitPlayground.motionX();
  Y = CircuitPlayground.motionY();
  Z = CircuitPlayground.motionZ();

  magnitude = X * X + Y * Y + Z * Z;
  movementChange = fabs(magnitude - previousMagnitude);
  previousMagnitude = magnitude;

  movementSum += movementChange;
  sampleCount++;

  unsigned long now = millis();

  if (now - lastCheckTime >= 1000) {
    float avgMovement = movementSum / sampleCount;
    String state = "NORMAL";

    if (avgMovement < breathingThreshold) {
      if (lowMovementStart == 0) {
        lowMovementStart = now;
      }

      unsigned long lowDuration = now - lowMovementStart;

      if (lowDuration >= 8000) {
        state = "ALERT";
        CircuitPlayground.clearPixels();
        setAllPixels(0xFF0000);   // red
        CircuitPlayground.playTone(1760, 100);
      } else if (lowDuration >= 5000) {
        state = "WARNING";
        CircuitPlayground.clearPixels();
        setAllPixels(0xFFFF00);   // yellow
      } else {
        state = "LOW_MOVEMENT_WAITING";
        CircuitPlayground.clearPixels();
        setAllPixels(0xFFFF00);   // yellow
      }

    } else {
      lowMovementStart = 0;
      state = "NORMAL";
      CircuitPlayground.clearPixels();
      setAllPixels(0x00FF00);     // green
    }

    float time_s = (now - startTime) / 1000.0;

    Serial.print(time_s);
    Serial.print(",");
    Serial.print(avgMovement);
    Serial.print(",");
    Serial.println(state);

    movementSum = 0;
    sampleCount = 0;
    lastCheckTime = now;
  }

  delay(50);
}