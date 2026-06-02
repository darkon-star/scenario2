#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

// Temperature thresholds
float warningTemp = 37.5;
float alertTemp = 38.0;

// Sensor values
float temperatureC;
float humidityPercent;

// Timing
unsigned long startTime;

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

  if (!aht.begin()) {
    Serial.println("Could not find AHT20. Check your wiring!");
    while (1) {
      delay(10);
    }
  }

  startTime = millis();

  Serial.println("time_s,temperature_C,state");
}

void loop() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  temperatureC = temp.temperature;
  humidityPercent = humidity.relative_humidity;

  String state = "NORMAL";

  if (temperatureC >= alertTemp) {
    state = "ALERT";

    CircuitPlayground.clearPixels();
    setAllPixels(0xFF0000);   // red
    alertSound();             // built-in speaker sound

  } else if (temperatureC >= warningTemp) {
    state = "WARNING";

    CircuitPlayground.clearPixels();
    setAllPixels(0xFFFF00);   // yellow

  } else {
    state = "NORMAL";

    CircuitPlayground.clearPixels();
    setAllPixels(0x00FF00);   // green
  }

  float time_s = (millis() - startTime) / 1000.0;

  Serial.print(time_s);
  Serial.print(",");
  Serial.print(temperatureC);
  Serial.print(",");
  Serial.println(state);

  delay(1000);
}