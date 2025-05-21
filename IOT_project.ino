#include <ESP8266WiFi.h>

// WiFi credentials
const char SSID []="Seminar Room 2G";
const char password[] = "student2";

#define LIGHT_SENSOR D4  // Analog pin for LDR
#define IR_SENSOR_1 D1     // GPIO5 - IR Sensor 1
#define IR_SENSOR_2 D2     // GPIO4 - IR Sensor 2
#define IR_SENSOR_3 D0     // GPIO16 - IR Sensor 3 (changed from D3/GPIO0)
#define LED_1 D5           // GPIO14 - LED 1
#define LED_2 D6           // GPIO12 - LED 2
#define LED_3 D7           // GPIO13 - LED 3

//#define LOW_LIGHT_THRESHOLD 500  // Analog threshold for dark condition
#define LOW_BRIGHTNESS 51        // 20% of 255
#define HIGH_BRIGHTNESS 255      // 100% brightness
#define DEBOUNCE_DELAY 50        // ms for IR sensor debouncing

void setup() {
  Serial.begin(115200); // Faster baud rate for debugging
  delay(1000);         // Delay for bootloader stability

  // Initialize pins
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(IR_SENSOR_1, INPUT);
  pinMode(IR_SENSOR_2, INPUT);
  pinMode(IR_SENSOR_3, INPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);

  // Set LEDs to OFF initially
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
}

void loop() {
  int lightLevel = digitalRead(LIGHT_SENSOR); // Read analog value (0-1023)
  Serial.print("Light Level: ");
  Serial.println(lightLevel);

  if (lightLevel == LOW) {
    // Bright outside: turn off all LEDs
    analogWrite(LED_1, 0);
    analogWrite(LED_2, 0);
    analogWrite(LED_3, 0);
    Serial.println("Bright outside: All LEDs OFF");
  } else {
    // Dark outside: set default low brightness
    analogWrite(LED_1, LOW_BRIGHTNESS);
    analogWrite(LED_2, LOW_BRIGHTNESS);
    analogWrite(LED_3, LOW_BRIGHTNESS);

    // Check IR sensors with debouncing
    if (digitalRead(IR_SENSOR_1) == LOW) {
        analogWrite(LED_1, HIGH_BRIGHTNESS);
        Serial.println("IR1 detected: LED1 full brightness");
    } else {
      analogWrite(LED_1, LOW_BRIGHTNESS);
    }

  
      if (digitalRead(IR_SENSOR_2) == LOW) {
        analogWrite(LED_2, HIGH_BRIGHTNESS);
        Serial.println("IR2 detected: LED2 full brightness");
      }
     else {
      analogWrite(LED_2, LOW_BRIGHTNESS);
    }

  
      if (digitalRead(IR_SENSOR_3) == LOW) {
        analogWrite(LED_3, HIGH_BRIGHTNESS);
        Serial.println("IR3 detected: LED3 full brightness");
      }
     else {
      analogWrite(LED_3, LOW_BRIGHTNESS);
    }
  }

  delay(200); // Main loop delay for stability
} 