#define BLYNK_TEMPLATE_ID "TMPL6MI5ybPyk"
#define BLYNK_TEMPLATE_NAME "Smart Street Lighting System"
#define BLYNK_FIRMWARE_VERSION "0.1.0"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Blynk Auth Token
char auth[] = "4V2t-LzFsEVtB3ghrPV830Xz6oJxk2mw";

// WiFi credentials
const char ssid[] = "Seminar Room 2G";
const char password[] = "student2";

// Pin definitions
#define LIGHT_SENSOR D8    // LDR on D4
#define IR_SENSOR_1 D1     // GPIO5 - IR Sensor 1
#define IR_SENSOR_2 D2     // GPIO4 - IR Sensor 2
#define IR_SENSOR_3 D0     // GPIO16 - IR Sensor 3
#define LED_1 D5           // GPIO14 - LED 1
#define LED_2 D6           // GPIO12 - LED 2
#define LED_3 D7           // GPIO13 - LED 3
#define TEMP_SENSOR D3     // DS18B20 on D3
#define CURRENT_SENSOR A0  // ACS712 on A0 (single sensor)
#define MODE_SWITCH D8     // Physical toggle switch for manual/auto mode

// Constants
#define LOW_LIGHT_THRESHOLD 500    // LDR threshold for night
#define LOW_BRIGHTNESS 51          // 20% of 255
#define HIGH_BRIGHTNESS 255        // 100% brightness
#define DEBOUNCE_DELAY 50          // Debounce delay
#define CURRENT_THRESHOLD 0.1      // Current threshold (Amps)

// DS18B20 setup
OneWire oneWire(TEMP_SENSOR);
DallasTemperature sensors(&oneWire);

// Blynk virtual pins
#define VPIN_LIGHT_LEVEL V0    // Display LDR value
#define VPIN_TEMP V1           // Display temperature
#define VPIN_CURRENT V2        // Current for active LED
#define VPIN_LED_1 V5          // Control LED 1 (manual mode)
#define VPIN_LED_2 V6          // Control LED 2 (manual mode)
#define VPIN_LED_3 V7          // Control LED 3 (manual mode)
#define VPIN_BRIGHTNESS_1 V8   // Intensity control LED 1
#define VPIN_BRIGHTNESS_2 V9   // Intensity control LED 2
#define VPIN_BRIGHTNESS_3 V10  // Intensity control LED 3
#define VPIN_WARNING V11       // Warning messages
#define VPIN_MODE V12          // Manual/Auto mode toggle

BlynkTimer timer;

// Variables
bool isManualMode = false;         // Mode flag (false = auto, true = manual)
int brightness[3] = {LOW_BRIGHTNESS, LOW_BRIGHTNESS, LOW_BRIGHTNESS}; // LED brightness
bool ledState[3] = {false, false, false}; // LED on/off state for manual mode
int activeLED = LED_1;             // Track the currently active LED for current sensing

void setup() {
  Serial.begin(115200);
  Serial.println("Started");
  Blynk.begin(auth, ssid, password);
  delay(1000);

  // Initialize pins
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR_SENSOR_1, INPUT);
  pinMode(IR_SENSOR_2, INPUT);
  pinMode(IR_SENSOR_3, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(MODE_SWITCH, INPUT_PULLUP); // Physical switch with internal pull-up

  // Initialize DS18B20
  sensors.begin();

  // Set LEDs to OFF initially
  analogWrite(LED_1, 0);
  analogWrite(LED_2, 0);
  analogWrite(LED_3, 0);

  // Timer for sensor updates and mode check
  timer.setInterval(2000L, sendSensorData);
  timer.setInterval(100L, checkPhysicalSwitch);
}

void sendSensorData() {
  // Read LDR
  int lightLevel = digitalRead(LIGHT_SENSOR);
  Blynk.virtualWrite(VPIN_LIGHT_LEVEL, lightLevel);

  // Read temperature
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  Blynk.virtualWrite(VPIN_TEMP, temp);

  // Read current for the active LED
  float current = readCurrent(activeLED);
  Blynk.virtualWrite(VPIN_CURRENT, current);

  // Check for faulty LED
  int ledIndex = activeLED - D5;
  if (current < CURRENT_THRESHOLD && brightness[ledIndex] > 0) {
    String warning = "LED " + String(ledIndex + 1) + " is faulty or missing!";
    Blynk.logEvent("warning", warning);
    Blynk.virtualWrite(VPIN_WARNING, warning);
  }
}

float readCurrent(int ledPin) {
  int ledIndex = ledPin - D5;
  // Turn off all LEDs except the active one
  for (int i = 0; i < 3; i++) {
    if (i != ledIndex) analogWrite(LED_1 + i, 0);
  }
  // Set active LED to its brightness
  analogWrite(ledPin, brightness[ledIndex]);
  delay(10); // Stabilize
  int sensorValue = analogRead(CURRENT_SENSOR);
  float voltage = sensorValue * (5.0 / 1023.0);
  float current = (voltage - 2.5) / 0.185; // ACS712 5A module
  return abs(current);
}

void checkPhysicalSwitch() {
  static bool lastSwitchState = HIGH;
  bool currentSwitchState = digitalRead(MODE_SWITCH);
  
  if (currentSwitchState != lastSwitchState && millis() > DEBOUNCE_DELAY) {
    isManualMode = (currentSwitchState == HIGH) ? false : true; // HIGH = auto, LOW = manual
    Blynk.virtualWrite(VPIN_MODE, isManualMode);
    Serial.println(isManualMode ? "Manual Mode" : "Auto Mode");
    lastSwitchState = currentSwitchState;
    controlLEDs();
  }
}

void controlLEDs() {
  int lightLevel = digitalRead(LIGHT_SENSOR);
  int ledPins[] = {LED_1, LED_2, LED_3};
  int irSensors[] = {IR_SENSOR_1, IR_SENSOR_2, IR_SENSOR_3};

  if (isManualMode) {
    // Manual mode: LEDs controlled by Blynk switches
    for (int i = 0; i < 3; i++) {
      analogWrite(ledPins[i], ledState[i] ? brightness[i] : 0);
      if (ledState[i]) activeLED = ledPins[i]; // Update active LED for current sensing
    }
  } else {
    // Auto mode: LEDs controlled by LDR
    if (lightLevel == LOW) {
      // Day: turn off LEDs
      for (int i = 0; i < 3; i++) {
        brightness[i] = 0;
        analogWrite(ledPins[i], 0);
      }
      activeLED = LED_1; // Default to LED_1 when all off
    } else {
      // Night: adjust brightness based on IR sensors
      for (int i = 0; i < 3; i++) {
        brightness[i] = (digitalRead(irSensors[i]) == LOW) ? HIGH_BRIGHTNESS : LOW_BRIGHTNESS;
        analogWrite(ledPins[i], brightness[i]);
        if (brightness[i] > 0) activeLED = ledPins[i]; // Update active LED
      }
    }
  }
}

// Blynk mode toggle
BLYNK_WRITE(VPIN_MODE) {
  isManualMode = param.asInt();
  Serial.println(isManualMode ? "Manual Mode (Blynk)" : "Auto Mode (Blynk)");
  controlLEDs();
}

// Blynk LED controls (manual mode only)
BLYNK_WRITE(VPIN_LED_1) {
  if (isManualMode) {
    ledState[0] = param.asInt();
    analogWrite(LED_1, ledState[0] ? brightness[0] : 0);
    if (ledState[0]) activeLED = LED_1;
  }
}

BLYNK_WRITE(VPIN_LED_2) {
  if (isManualMode) {
    ledState[1] = param.asInt();
    analogWrite(LED_2, ledState[1] ? brightness[1] : 0);
    if (ledState[1]) activeLED = LED_2;
  }
}

BLYNK_WRITE(VPIN_LED_3) {
  if (isManualMode) {
    ledState[2] = param.asInt();
    analogWrite(LED_3, ledState[2] ? brightness[2] : 0); // Fixed missing parenthesis
    if (ledState[2]) activeLED = LED_3;
  }
}

BLYNK_WRITE(VPIN_BRIGHTNESS_1) {
  if (isManualMode) {
    brightness[0] = param.asInt();
    analogWrite(LED_1, ledState[0] ? brightness[0] : 0);
    if (ledState[0]) activeLED = LED_1;
  }
}

BLYNK_WRITE(VPIN_BRIGHTNESS_2) {
  if (isManualMode) {
    brightness[1] = param.asInt();
    analogWrite(LED_2, ledState[1] ? brightness[1] : 0);
    if (ledState[1]) activeLED = LED_2;
  }
}

BLYNK_WRITE(VPIN_BRIGHTNESS_3) {
  if (isManualMode) {
    brightness[2] = param.asInt();
    analogWrite(LED_3, ledState[2] ? brightness[2] : 0);
    if (ledState[2]) activeLED = LED_3;
  }
}

void loop() {
  Blynk.run();
  timer.run();
  controlLEDs();
}