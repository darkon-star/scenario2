#include <Adafruit_CircuitPlayground.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

const int flexPin = A2;  // flex sensor is connected to A2

float smoothValue = 0;
float alpha = 0.15;  // smoothing factor

float highValue = 0;
float lowValue = 1023;
float flexChange = 0;  // range of the flex sensor signal in one time window

const unsigned long windowTime = 3000;  // 3-second window for breathing detection
unsigned long windowStart = 0;

const float BREATHING_THRESHOLD = 5.0;  // minimum flex change needed to count as breathing movement

int lowWindowCount = 0;
int normalWindowCount = 0;

const int WARNING_LOW_WINDOWS = 3;  // about 9 seconds of low movement
const int ALERT_LOW_WINDOWS = 7;  // about 21 seconds of low movement
const int RECOVERY_NORMAL_WINDOWS = 2;  // needs 2 normal windows to return to normal

float temperatureC = 0;

const float TEMP_WARNING = 34.5;
const float TEMP_ALERT = 35.0;

String breathingState = "NORMAL";
String tempState = "NORMAL";
String overallState = "NORMAL";

bool systemOn = false;  // used to control whether the system is running

bool lastRightButton = false;
bool lastLeftButton = false;

unsigned long startTime = 0;

unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 200;  // print data every 200 ms

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

  // right button starts the system
  if (rightButton && !lastRightButton) {
    startSystem();
  }

  // left button stops the system
  if (leftButton && !lastLeftButton) {
    stopSystem();
  }

  lastRightButton = rightButton;
  lastLeftButton = leftButton;

  // if the system is off, do nothing except keep the lights off
  if (!systemOn) {
    CircuitPlayground.clearPixels();
    delay(50);
    return;
  }

  int raw = analogRead(flexPin);

  // smooth the raw flex sensor value to reduce small noise
  smoothValue = alpha * raw + (1 - alpha) * smoothValue;

  // keep updating the highest and lowest smooth value in the current 3-second window
  if (smoothValue > highValue) {
    highValue = smoothValue;
  }

  if (smoothValue < lowValue) {
    lowValue = smoothValue;
  }

  // read temperature from AHT20
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  temperatureC = temp.temperature;

  updateTempState();

  // every 3 seconds, calculate the range of the flex signal
  if (currentTime - windowStart >= windowTime) {
    flexChange = highValue - lowValue;
    updateBreathingState();

   // reset the window for the next 3 seconds
    highValue = smoothValue;
    lowValue = smoothValue;
    windowStart = currentTime;
  }

  updateOverallState();
  updateOutput();

  // print only the main values need for checking and recording
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

  // initialise the flex sensor reading when the system starts
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
  // if the flex sensor changes enough in 3 seconds, count it as breathing movement
  if (flexChange >= BREATHING_THRESHOLD) {
    normalWindowCount++;

    // one normal window is not enough to reset the alarm
    // this avoids one accidental movement cancelling the warning
    if (normalWindowCount >= RECOVERY_NORMAL_WINDOWS) {
      lowWindowCount = 0;
      breathingState = "NORMAL";
    }
  }

  else {
    // if the flex change is too small, count it as one low movement window
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
  // warning and alert thresholds of temperature sensor
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
  // alert has the highest priority, then warning, then low movement
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

  // breathing alerts are checked first because they are more urgent in our prototype
  if (breathingState == "ALERT") {
    setAllPixels(255, 0, 0);        // red
    playBreathingAlertSound();
  }

  else if (tempState == "ALERT") {
    setAllPixels(180, 0, 255);      // purple
    playTemperatureAlertSound();
  }

  else if (breathingState == "WARNING") {
    setAllPixels(255, 80, 0);       // orange
    playBreathingWarningSound();
  }

  else if (tempState == "WARNING") {
    setAllPixels(255, 255, 0);      // yellow
    playTemperatureWarningSound();
  }

  else if (breathingState == "LOW_MOVEMENT") {
    setAllPixels(255, 255, 255);    // white
  }

  else {
    setAllPixels(0, 255, 0);        // green
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
  // set all 10 LEDs to the same colour
  for (int i = 0; i < 10; i++) {
    CircuitPlayground.setPixelColor(i, r, g, b);
  }
}