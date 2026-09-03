/*
 * ======================================================================================
 * GREENHOUSE CLIMATE CONTROL - ARDUINO IOT CLOUD FIRMWARE (ESP32)
 * ======================================================================================
 * 
 * Hardware Compatibility: 100% Identical to Existing Build (No Hardware Modifications)
 * Platform: ESP32 DevKit V1
 * Cloud Platform: Arduino IoT Cloud (Free Tier - 4 Variables Limit)
 * 
 * Configured Cloud Variables (4/4 Free Tier - Basic Types):
 *   1. humidity      (Floating Point Number / float, Read-Only, On Change)
 *   2. potentiometer (Integer Number / int, Read-Only, On Change)
 *   3. roofVent      (Boolean / bool, Read & Write, On Change) -> Callback: onRoofVentChange()
 *   4. sideVent      (Boolean / bool, Read & Write, On Change) -> Callback: onSideVentChange()
 * 
 * Pin Mapping Summary:
 *   - GPIO 34 : Potentiometer Analog Input (ADC1_CH6, 0-100%)
 *   - GPIO 4  : DHT11 Sensor (Humidity & Temperature Data)
 *   - GPIO 14 : HW-416-B PIR Motion Sensor (Digital In)
 *   - GPIO 26 : Button 1 (Roof Vent Manual Override Toggle, Active LOW)
 *   - GPIO 27 : Button 2 (Side Vent Manual Override Toggle, Active LOW)
 *   - GPIO 18 : SG90 Micro-Servo 1 (Roof Vent, 50 Hz PWM)
 *   - GPIO 19 : SG90 Micro-Servo 2 (Side Vent / Shade, 50 Hz PWM)
 *   - GPIO 2  : Built-in Blue Status LED (Solid = Connected to Cloud)
 * ======================================================================================
 */

#include "thingProperties.h"
#include <DHT.h>
#include <ESP32Servo.h>

// ======================================================================================
// HARDWARE PIN ASSIGNMENTS (UNCHANGED)
// ======================================================================================
namespace Pins {
    constexpr uint8_t POT_PIN        = 34; // Potentiometer Wiper (0-3.3V, ADC1)
    constexpr uint8_t DHT_PIN        = 4;  // DHT11 Data Pin
    constexpr uint8_t PIR_PIN        = 14; // PIR Motion Sensor Signal
    constexpr uint8_t BUTTON_1_PIN   = 26; // Button 1: Roof Vent Toggle
    constexpr uint8_t BUTTON_2_PIN   = 27; // Button 2: Side Vent Toggle
    constexpr uint8_t SERVO_1_PIN    = 18; // Servo 1: Roof Vent PWM
    constexpr uint8_t SERVO_2_PIN    = 19; // Servo 2: Side Vent PWM
    constexpr uint8_t STATUS_LED_PIN = 2;  // Onboard Blue Status LED
}

// ======================================================================================
// CALIBRATION & TIMING CONSTANTS
// ======================================================================================
namespace Config {
    constexpr int SERVO_VENT_CLOSED_ANGLE   = 0;   // Degrees (Closed)
    constexpr int SERVO_VENT_OPEN_ANGLE     = 90;  // Degrees (Fully Open)

    constexpr unsigned long DHT_READ_INTERVAL_MS = 2500; // DHT11 sample period (>= 2 sec)
    constexpr unsigned long POT_SAMPLE_MS        = 100;  // Potentiometer polling period
    constexpr unsigned long BUTTON_DEBOUNCE_MS   = 50;   // Switch debounce window
    constexpr int POT_DELTA_TRIGGER              = 2;    // Min % change to sync with Cloud
}

// ======================================================================================
// HARDWARE DRIVERS & ACTUATORS
// ======================================================================================
DHT dhtSensor(Pins::DHT_PIN, DHT11);
Servo servoRoof; // Roof Vent Actuator (Servo 1)
Servo servoSide; // Side Vent Actuator (Servo 2)

// ======================================================================================
// RUNTIME STATE VARIABLES
// ======================================================================================
float localTemperature      = 0.0f;
bool  sensorValid           = false;
bool  lastMotionState       = false;

int   currentPotRaw         = 0;
int   currentPotPercent     = 0;
int   lastSyncedPotPercent  = -1;

unsigned long lastDhtReadTime = 0;
unsigned long lastPotSampleTime = 0;

// Button Debouncing Structure
struct DebouncedButton {
    uint8_t pin;
    int lastReading;
    int stableState;
    unsigned long lastDebounceTime;
};

DebouncedButton btn1 = { Pins::BUTTON_1_PIN, HIGH, HIGH, 0 };
DebouncedButton btn2 = { Pins::BUTTON_2_PIN, HIGH, HIGH, 0 };

// ======================================================================================
// FORWARD DECLARATIONS
// ======================================================================================
void readSensors();
void readPotentiometer();
void handleButtons();
void updateDebouncedButton(DebouncedButton &btn, uint8_t btnNumber);
void applyServoPositions();

// ======================================================================================
// SETUP
// ======================================================================================
void setup() {
    // 1. Initialize Serial Monitor
    Serial.begin(115200);
    delay(1500); // Give serial monitor time to attach
    Serial.println("\n==================================================");
    Serial.println(" GREENHOUSE CLIMATE CONTROL - ARDUINO IOT CLOUD");
    Serial.println("==================================================");

    // 2. Configure Pin Modes
    pinMode(Pins::POT_PIN, INPUT);
    pinMode(Pins::PIR_PIN, INPUT);
    pinMode(Pins::BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(Pins::BUTTON_2_PIN, INPUT_PULLUP);
    pinMode(Pins::STATUS_LED_PIN, OUTPUT);
    digitalWrite(Pins::STATUS_LED_PIN, LOW);

    // 3. Initialize DHT11 Sensor
    dhtSensor.begin();
    Serial.println("[DHT11] Sensor initialized on GPIO " + String(Pins::DHT_PIN));
    Serial.println("[POT] Analog potentiometer on GPIO " + String(Pins::POT_PIN));

    // 4. Initialize Servos (Standard 50 Hz PWM)
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    servoRoof.setPeriodHertz(50);
    servoSide.setPeriodHertz(50);

    servoRoof.attach(Pins::SERVO_1_PIN, 500, 2400);
    servoSide.attach(Pins::SERVO_2_PIN, 500, 2400);

    // Default: Close both vents initially
    roofVent = false;
    sideVent = false;
    servoRoof.write(Config::SERVO_VENT_CLOSED_ANGLE);
    servoSide.write(Config::SERVO_VENT_CLOSED_ANGLE);

    Serial.println("[SERVOS] Servo 1 (Roof Vent) on GPIO " + String(Pins::SERVO_1_PIN));
    Serial.println("[SERVOS] Servo 2 (Side Vent) on GPIO " + String(Pins::SERVO_2_PIN));

    // 5. Initialize Arduino IoT Cloud Properties (defined in thingProperties.h)
    initProperties();

    // 6. Connect to Arduino IoT Cloud
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);

    // Debug Message Level (0 = Errors only, 2 = Normal info, 4 = Verbose)
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();

    Serial.println("[SYSTEM] Initialization finished. Connecting to Cloud...\n");
}

// ======================================================================================
// MAIN LOOP
// ======================================================================================
void loop() {
    // 1. Maintain background synchronization with Arduino IoT Cloud
    ArduinoCloud.update();

    // 2. Visual Connection Status on Onboard LED
    if (ArduinoCloud.connected()) {
        digitalWrite(Pins::STATUS_LED_PIN, HIGH);
    } else {
        digitalWrite(Pins::STATUS_LED_PIN, LOW);
    }

    // 3. Read Physical Sensors (DHT11 & PIR)
    readSensors();

    // 4. Read Analog Potentiometer
    readPotentiometer();

    // 5. Check Debounced Physical Buttons
    handleButtons();

    // Small yield to FreeRTOS watchdog
    delay(5);
}

// ======================================================================================
// SENSOR READING (DHT11 & PIR)
// ======================================================================================
void readSensors() {
    unsigned long now = millis();

    // Read DHT11 every >= 2.5 seconds
    if (now - lastDhtReadTime >= Config::DHT_READ_INTERVAL_MS) {
        lastDhtReadTime = now;

        float h = dhtSensor.readHumidity();
        float t = dhtSensor.readTemperature();

        if (isnan(h) || isnan(t)) {
            sensorValid = false;
            Serial.println("[DHT11 WARN] Failed to read sensor! Check wiring and pull-up.");
        } else {
            sensorValid = true;
            localTemperature = t;

            // Update Cloud Variable (Variable 1/4)
            humidity = h;

            Serial.printf("[DHT11] Humidity: %.1f %% | Temp (Local): %.1f C\n", h, t);
        }
    }

    // Read PIR Sensor (Instant edge detection for local monitoring)
    bool rawMotion = (digitalRead(Pins::PIR_PIN) == HIGH);
    if (rawMotion != lastMotionState) {
        lastMotionState = rawMotion;
        Serial.printf("[PIR] Motion Status: %s\n", lastMotionState ? "MOTION DETECTED" : "CLEAR");
    }
}

// ======================================================================================
// POTENTIOMETER ANALOG READING (ADC1)
// ======================================================================================
void readPotentiometer() {
    unsigned long now = millis();
    if (now - lastPotSampleTime < Config::POT_SAMPLE_MS) {
        return;
    }
    lastPotSampleTime = now;

    // Read 12-bit ADC (0 - 4095)
    int raw = analogRead(Pins::POT_PIN);
    currentPotRaw = raw;

    // Map to percentage (0 - 100%)
    int pct = map(raw, 0, 4095, 0, 100);
    pct = constrain(pct, 0, 100);
    currentPotPercent = pct;

    // If change exceeds threshold, sync with Arduino Cloud
    if (abs(currentPotPercent - lastSyncedPotPercent) >= Config::POT_DELTA_TRIGGER) {
        lastSyncedPotPercent = currentPotPercent;

        // Update Cloud Variable (Variable 2/4)
        potentiometer = currentPotPercent;

        Serial.printf("[POT] Knob Position: %d %% (Raw: %d)\n", currentPotPercent, currentPotRaw);
    }
}

// ======================================================================================
// PHYSICAL BUTTON PROCESSING (Debounce + Bidirectional Cloud Sync)
// ======================================================================================
void handleButtons() {
    updateDebouncedButton(btn1, 1);
    updateDebouncedButton(btn2, 2);

    // Simultaneous Button Press: Emergency Close Both Vents
    if (btn1.stableState == LOW && btn2.stableState == LOW) {
        if (roofVent || sideVent) {
            Serial.println("[BUTTONS] Both buttons pressed -> Resetting both vents to CLOSED");
            roofVent = false;
            sideVent = false;
            servoRoof.write(Config::SERVO_VENT_CLOSED_ANGLE);
            servoSide.write(Config::SERVO_VENT_CLOSED_ANGLE);
        }
    }
}

void updateDebouncedButton(DebouncedButton &btn, uint8_t btnNumber) {
    int reading = digitalRead(btn.pin);

    if (reading != btn.lastReading) {
        btn.lastDebounceTime = millis();
    }

    if ((millis() - btn.lastDebounceTime) > Config::BUTTON_DEBOUNCE_MS) {
        if (reading != btn.stableState) {
            btn.stableState = reading;

            if (btn.stableState == LOW) { // Button Pressed
                Serial.printf("[BUTTON %d] Pressed (Manual Toggle)\n", btnNumber);

                if (btnNumber == 1) {
                    // Toggle Roof Vent variable (Variable 3/4)
                    roofVent = !roofVent;
                    servoRoof.write(roofVent ? Config::SERVO_VENT_OPEN_ANGLE : Config::SERVO_VENT_CLOSED_ANGLE);
                    Serial.printf("[LOCAL] Roof Vent set to: %s (%d deg)\n", 
                                  roofVent ? "OPEN" : "CLOSED", 
                                  roofVent ? Config::SERVO_VENT_OPEN_ANGLE : Config::SERVO_VENT_CLOSED_ANGLE);
                } else if (btnNumber == 2) {
                    // Toggle Side Vent variable (Variable 4/4)
                    sideVent = !sideVent;
                    servoSide.write(sideVent ? Config::SERVO_VENT_OPEN_ANGLE : Config::SERVO_VENT_CLOSED_ANGLE);
                    Serial.printf("[LOCAL] Side Vent set to: %s (%d deg)\n", 
                                  sideVent ? "OPEN" : "CLOSED", 
                                  sideVent ? Config::SERVO_VENT_OPEN_ANGLE : Config::SERVO_VENT_CLOSED_ANGLE);
                }
            }
        }
    }
    btn.lastReading = reading;
}

// ======================================================================================
// ARDUINO IOT CLOUD CALLBACK HANDLERS (Dashboard -> ESP32)
// ======================================================================================

/*
 * Triggered automatically when 'roofVent' is toggled from the Cloud Dashboard.
 */
void onRoofVentChange() {
    Serial.printf("[CLOUD EVENT] Roof Vent Switch changed -> %s\n", roofVent ? "OPEN" : "CLOSED");
    if (roofVent) {
        servoRoof.write(Config::SERVO_VENT_OPEN_ANGLE);
    } else {
        servoRoof.write(Config::SERVO_VENT_CLOSED_ANGLE);
    }
}

/*
 * Triggered automatically when 'sideVent' is toggled from the Cloud Dashboard.
 */
void onSideVentChange() {
    Serial.printf("[CLOUD EVENT] Side Vent Switch changed -> %s\n", sideVent ? "OPEN" : "CLOSED");
    if (sideVent) {
        servoSide.write(Config::SERVO_VENT_OPEN_ANGLE);
    } else {
        servoSide.write(Config::SERVO_VENT_CLOSED_ANGLE);
    }
}
