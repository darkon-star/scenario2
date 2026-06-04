#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

const int flexPin = A2;

float smoothValue = 0;
float alpha = 0.15;

float highValue = 0;
float lowValue = 1023;
float flexChange = 0;

const unsigned long windowTime = 3000;
unsigned long windowStart = 0;

const float BREATHING_THRESHOLD = 5.0;

int lowWindowCount = 0;
int normalWindowCount = 0;

const int WARNING_LOW_WINDOWS = 3;
const int ALERT_LOW_WINDOWS = 7;
const int RECOVERY_NORMAL_WINDOWS = 2;

float temperatureC = 0;

const float TEMP_WARNING = 34.5;
const float TEMP_ALERT = 35.0;

String breathingState = "NORMAL";
String tempState = "NORMAL";
String overallState = "NORMAL";

bool systemOn = false;

bool lastRightButton = false;
bool lastLeftButton = false;

unsigned long startTime = 0;

unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 200;

void setup() {
  CircuitPlayground.begin();
  Serial.begin(115200);
  delay(2000);

  aht.begin();

  CircuitPlayground.clearPixels();

  Serial.println("Press RIGHT to start, LEFT to stop");
}

void loop() {
  unsigned long currentTime = millis();

  bool rightButton = CircuitPlayground.rightButton();
  bool leftButton = CircuitPlayground.leftButton();

  if (rightButton && !lastRightButton) {
    startSystem();
  }

  if (leftButton && !lastLeftButton) {
    stopSystem();
  }

  lastRightButton = rightButton;
  lastLeftButton = leftButton;

  if (!systemOn) {
    CircuitPlayground.clearPixels();
    delay(50);
    return;
  }

  int raw = analogRead(flexPin);

  smoothValue = alpha * raw + (1 - alpha) * smoothValue;

  if (smoothValue > highValue) {
    highValue = smoothValue;
  }

  if (smoothValue < lowValue) {
    lowValue = smoothValue;
  }

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  temperatureC = temp.temperature;

  updateTempState();

  if (currentTime - windowStart >= windowTime) {
    flexChange = highValue - lowValue;
    updateBreathingState();

    highValue = smoothValue;
    lowValue = smoothValue;
    windowStart = currentTime;
  }

  updateOverallState();
  updateOutput();

  if (currentTime - lastPrint >= PRINT_INTERVAL) {
    lastPrint = currentTime;

    Serial.print((currentTime - startTime) / 1000.0, 1);
    Serial.print(",");
    Serial.print(temperatureC, 1);
    Serial.print(",");
    Serial.print(tempState);
    Serial.print(",");
    Serial.print(smoothValue, 1);
    Serial.print(",");
    Serial.println(breathingState);
  }

  delay(20);
}

void startSystem() {
  systemOn = true;

  startTime = millis();
  lastPrint = millis();

  int raw = analogRead(flexPin);
  smoothValue = raw;

  highValue = smoothValue;
  lowValue = smoothValue;
  flexChange = 0;

  lowWindowCount = 0;
  normalWindowCount = 0;

  breathingState = "NORMAL";
  tempState = "NORMAL";
  overallState = "NORMAL";

  windowStart = millis();

  Serial.println("time_s,temperature_C,TEMP_STATE,smooth,BREATHING_STATE");
}

void stopSystem() {
  systemOn = false;

  CircuitPlayground.clearPixels();

  lowWindowCount = 0;
  normalWindowCount = 0;

  breathingState = "NORMAL";
  tempState = "NORMAL";
  overallState = "NORMAL";
}

void updateBreathingState() {
  if (flexChange >= BREATHING_THRESHOLD) {
    normalWindowCount++;

    if (normalWindowCount >= RECOVERY_NORMAL_WINDOWS) {
      lowWindowCount = 0;
      breathingState = "NORMAL";
    }
  }

  else {
    lowWindowCount++;
    normalWindowCount = 0;

    if (lowWindowCount >= ALERT_LOW_WINDOWS) {
      breathingState = "ALERT";
    }

    else if (lowWindowCount >= WARNING_LOW_WINDOWS) {
      breathingState = "WARNING";
    }

    else {
      breathingState = "LOW_MOVEMENT";
    }
  }
}

void updateTempState() {
  if (temperatureC >= TEMP_ALERT) {
    tempState = "ALERT";
  }

  else if (temperatureC >= TEMP_WARNING) {
    tempState = "WARNING";
  }

  else {
    tempState = "NORMAL";
  }
}

void updateOverallState() {
  if (breathingState == "ALERT" || tempState == "ALERT") {
    overallState = "ALERT";
  }

  else if (breathingState == "WARNING" || tempState == "WARNING") {
    overallState = "WARNING";
  }

  else if (breathingState == "LOW_MOVEMENT") {
    overallState = "LOW_MOVEMENT";
  }

  else {
    overallState = "NORMAL";
  }
}

void updateOutput() {
  CircuitPlayground.clearPixels();

  if (breathingState == "ALERT") {
    setAllPixels(255, 0, 0);
    playBreathingAlertSound();
  }

  else if (tempState == "ALERT") {
    setAllPixels(180, 0, 255);
    playTemperatureAlertSound();
  }

  else if (breathingState == "WARNING") {
    setAllPixels(255, 80, 0);
    playBreathingWarningSound();
  }

  else if (tempState == "WARNING") {
    setAllPixels(255, 255, 0);
    playTemperatureWarningSound();
  }

  else if (breathingState == "LOW_MOVEMENT") {
    setAllPixels(255, 255, 255);
  }

  else {
    setAllPixels(0, 255, 0);
  }
}

void playBreathingWarningSound() {
  CircuitPlayground.playTone(500, 200);
}

void playBreathingAlertSound() {
  CircuitPlayground.playTone(700, 120);
  delay(80);
  CircuitPlayground.playTone(700, 120);
  delay(80);
  CircuitPlayground.playTone(700, 120);
}

void playTemperatureWarningSound() {
  CircuitPlayground.playTone(1400, 120);
  delay(100);
  CircuitPlayground.playTone(1400, 120);
}

void playTemperatureAlertSound() {
  CircuitPlayground.playTone(2000, 500);
}

void setAllPixels(int r, int g, int b) {
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, r, g, b);
  }
}