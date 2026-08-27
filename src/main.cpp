/*
 * ============================================================================
 * GREENHOUSE CLIMATE-CONTROL SIMULATION (STANDALONE ESP32 CONTROLLER)
 * ============================================================================
 * Platform: ESP32 Dev Module (PlatformIO / Arduino Framework)
 * Hardware:
 *   - 1x ESP32 DevKit (Wi-Fi + MQTT Client + Controller Brain)
 *   - 1x DHT11 Temperature & Humidity Sensor (GPIO 4)
 *   - 1x HW-416-B PIR Motion Sensor (GPIO 14)
 *   - 1x Button 1: Roof Vent Manual Override (GPIO 26, INPUT_PULLUP)
 *   - 1x Button 2: Side Vent Manual Override (GPIO 27, INPUT_PULLUP)
 *   - 1x SG90 Micro-Servo 1: Roof Vent (GPIO 18, 50 Hz PWM)
 *   - 1x SG90 Micro-Servo 2: Side Vent / Shade (GPIO 19, 50 Hz PWM)
 *   - External 5V Power Supply powering both servos & sensors with Common GND
 * ============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <MQTT.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP32Servo.h>

#include "config.h"
#include "secrets.h"

// ============================================================================
// SYSTEM OBJECTS & HARDWARE DRIVERS
// ============================================================================
WiFiClient netClient;
MQTTClient mqttClient(1024); // 1 KB payload buffer for JSON messages
DHT dhtSensor(Pins::DHT_PIN, DHT11);

// Direct Servo Drivers (Driven via ESP32 Hardware PWM Timers)
Servo servoRoof; // Servo 1 (Roof Vent)
Servo servoSide; // Servo 2 (Side Vent / Shade)

// ============================================================================
// SYSTEM STATE VARIABLES
// ============================================================================
enum ControlMode {
    MODE_AUTO,
    MODE_MANUAL
};

ControlMode currentMode = MODE_AUTO;

// Climate & Sensor Readings
float currentTemperature = 0.0f;
float currentHumidity    = 0.0f;
bool  sensorValid        = false;
bool  currentMotionState = false;

// Actuator Angles (0 to 180 degrees)
int servo1RoofAngle = Config::SERVO_VENT_CLOSED_ANGLE;
int servo2SideAngle = Config::SERVO_VENT_CLOSED_ANGLE;

// Non-blocking Timing Trackers (millis)
unsigned long lastDhtReadTime      = 0;
unsigned long lastTelemetryPubTime = 0;
unsigned long lastMqttRetryTime    = 0;
unsigned long lastWifiCheckTime    = 0;

// Button Debounce State Trackers
struct DebouncedButton {
    uint8_t pin;
    int lastReading;
    int stableState;
    unsigned long lastDebounceTime;
};

DebouncedButton btn1 = { Pins::BUTTON_1_PIN, HIGH, HIGH, 0 };
DebouncedButton btn2 = { Pins::BUTTON_2_PIN, HIGH, HIGH, 0 };

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
void setupWiFi();
void checkWiFiConnection();
void connectMQTT();
void onMqttMessageReceived(String &topic, String &payload);
void readSensors();
void handleButtons();
void updateDebouncedButton(DebouncedButton &btn, uint8_t btnNumber);
void applyControlLogic();
void publishTelemetry();
void setRoofVent(int angle, const char* source = "LOCAL");
void setSideVent(int angle, const char* source = "LOCAL");

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // 1. Initialize Serial Monitor
    Serial.begin(115200);
    delay(500);
    Serial.println("\n==================================================");
    Serial.println("   GREENHOUSE CLIMATE-CONTROL (STANDALONE ESP32)");
    Serial.println("==================================================");

    // 2. Configure GPIO Pins
    pinMode(Pins::PIR_PIN, INPUT);
    pinMode(Pins::BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(Pins::BUTTON_2_PIN, INPUT_PULLUP);
    pinMode(Pins::STATUS_LED_PIN, OUTPUT);
    digitalWrite(Pins::STATUS_LED_PIN, LOW);

    // 3. Initialize DHT11 Sensor
    dhtSensor.begin();
    Serial.println("[DHT11] Sensor initialized on GPIO " + String(Pins::DHT_PIN));

    // 4. Initialize Servos with Dedicated ESP32 Hardware PWM Timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    servoRoof.setPeriodHertz(50); // Standard 50 Hz PWM for SG90
    servoSide.setPeriodHertz(50);

    servoRoof.attach(Pins::SERVO_1_PIN, 500, 2400); // Standard micro-servo pulse range
    servoSide.attach(Pins::SERVO_2_PIN, 500, 2400);

    // Set initial positions (Closed)
    setRoofVent(Config::SERVO_VENT_CLOSED_ANGLE, "INIT");
    setSideVent(Config::SERVO_VENT_CLOSED_ANGLE, "INIT");

    Serial.println("[SERVOS] Servo 1 (Roof) attached to GPIO " + String(Pins::SERVO_1_PIN));
    Serial.println("[SERVOS] Servo 2 (Side) attached to GPIO " + String(Pins::SERVO_2_PIN));

    // 5. Initialize Wi-Fi & MQTT Client
    setupWiFi();
    mqttClient.begin(MQTT_BROKER_IP, MQTT_BROKER_PORT, netClient);
    mqttClient.onMessage(onMqttMessageReceived);
    
    // Set Last Will & Testament (LWT)
    mqttClient.setWill(Topics::LWT_STATUS, "offline", true, 1);

    Serial.println("[SYSTEM] Initialization complete. Entering main loop.\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    // 1. Maintain Network & MQTT Connectivity
    checkWiFiConnection();
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            unsigned long now = millis();
            if (now - lastMqttRetryTime >= Config::MQTT_RECONNECT_INTERVAL_MS) {
                lastMqttRetryTime = now;
                connectMQTT();
            }
        } else {
            mqttClient.loop(); // Process incoming MQTT packets & keepalives
        }
    }

    // 2. Read Sensors (DHT11 & PIR)
    readSensors();

    // 3. Process Debounced Push Buttons
    handleButtons();

    // 4. Evaluate Automatic Climate Control Rules
    applyControlLogic();

    // 5. Stream Periodic MQTT Telemetry
    publishTelemetry();

    // Small delay to yield to FreeRTOS watchdog
    delay(5);
}

// ============================================================================
// WI-FI MANAGEMENT (Non-Blocking Auto-Reconnect)
// ============================================================================
void setupWiFi() {
    Serial.printf("[Wi-Fi] Connecting to SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void checkWiFiConnection() {
    unsigned long now = millis();
    if (now - lastWifiCheckTime >= 1000) {
        lastWifiCheckTime = now;
        if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(Pins::STATUS_LED_PIN, HIGH);
        } else {
            digitalWrite(Pins::STATUS_LED_PIN, LOW);
            if (WiFi.status() != WL_DISCONNECTED && WiFi.status() != WL_IDLE_STATUS) {
                WiFi.disconnect();
                WiFi.reconnect();
            }
        }
    }
}

// ============================================================================
// MQTT CONNECTION & RECONNECTION
// ============================================================================
void connectMQTT() {
    Serial.printf("[MQTT] Attempting connection to broker %s:%d...\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);

    bool connected = false;
    if (strlen(MQTT_USERNAME) > 0) {
        connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
    } else {
        connected = mqttClient.connect(MQTT_CLIENT_ID);
    }

    if (connected) {
        Serial.println("[MQTT] Connected successfully!");

        // Publish LWT Online Status (Retained)
        mqttClient.publish(Topics::LWT_STATUS, "online", true, 1);

        // Subscribe to inbound control topics
        mqttClient.subscribe(Topics::CMD_SERVO1_SET, 1);
        mqttClient.subscribe(Topics::CMD_SERVO2_SET, 1);
        mqttClient.subscribe(Topics::CMD_MODE_SET, 1);

        Serial.printf("[MQTT] Subscribed to:\n  - %s\n  - %s\n  - %s\n", 
                      Topics::CMD_SERVO1_SET, Topics::CMD_SERVO2_SET, Topics::CMD_MODE_SET);

        // Broadcast initial states
        mqttClient.publish(Topics::SYSTEM_MODE, (currentMode == MODE_AUTO) ? "AUTO" : "MANUAL", true, 1);
        mqttClient.publish(Topics::SERVO1_STATUS, (servo1RoofAngle > 0) ? "OPEN" : "CLOSED", true, 1);
        mqttClient.publish(Topics::SERVO2_STATUS, (servo2SideAngle > 0) ? "OPEN" : "CLOSED", true, 1);
    } else {
        Serial.printf("[MQTT] Connection failed (Error code: %d). Retrying in %lu ms\n", 
                      mqttClient.lastError(), Config::MQTT_RECONNECT_INTERVAL_MS);
    }
}

// ============================================================================
// MQTT INBOUND MESSAGE HANDLER
// ============================================================================
void onMqttMessageReceived(String &topic, String &payload) {
    Serial.printf("[MQTT IN] Topic: %s | Payload: %s\n", topic.c_str(), payload.c_str());
    payload.trim();

    // 1. Mode Change Command
    if (topic == Topics::CMD_MODE_SET) {
        if (payload.equalsIgnoreCase("AUTO")) {
            currentMode = MODE_AUTO;
            Serial.println("[MODE] Switched to AUTOMATIC Climate Control");
            mqttClient.publish(Topics::SYSTEM_MODE, "AUTO", true, 1);
        } else if (payload.equalsIgnoreCase("MANUAL")) {
            currentMode = MODE_MANUAL;
            Serial.println("[MODE] Switched to MANUAL Override");
            mqttClient.publish(Topics::SYSTEM_MODE, "MANUAL", true, 1);
        }
        return;
    }

    // 2. Servo 1 (Roof Vent) Command
    if (topic == Topics::CMD_SERVO1_SET) {
        currentMode = MODE_MANUAL;
        mqttClient.publish(Topics::SYSTEM_MODE, "MANUAL", true, 1);

        if (payload.equalsIgnoreCase("OPEN")) {
            setRoofVent(Config::SERVO_VENT_OPEN_ANGLE, "MQTT_CMD");
        } else if (payload.equalsIgnoreCase("CLOSE")) {
            setRoofVent(Config::SERVO_VENT_CLOSED_ANGLE, "MQTT_CMD");
        } else {
            int requestedAngle = payload.toInt();
            if (requestedAngle >= 0 && requestedAngle <= 180) {
                setRoofVent(requestedAngle, "MQTT_CMD");
            } else {
                Serial.println("[WARN] Invalid Servo 1 angle: " + payload);
            }
        }
        return;
    }

    // 3. Servo 2 (Side Vent) Command
    if (topic == Topics::CMD_SERVO2_SET) {
        currentMode = MODE_MANUAL;
        mqttClient.publish(Topics::SYSTEM_MODE, "MANUAL", true, 1);

        if (payload.equalsIgnoreCase("OPEN")) {
            setSideVent(Config::SERVO_VENT_OPEN_ANGLE, "MQTT_CMD");
        } else if (payload.equalsIgnoreCase("CLOSE")) {
            setSideVent(Config::SERVO_VENT_CLOSED_ANGLE, "MQTT_CMD");
        } else {
            int requestedAngle = payload.toInt();
            if (requestedAngle >= 0 && requestedAngle <= 180) {
                setSideVent(requestedAngle, "MQTT_CMD");
            } else {
                Serial.println("[WARN] Invalid Servo 2 angle: " + payload);
            }
        }
        return;
    }
}

// ============================================================================
// SENSOR READING (DHT11 & PIR)
// ============================================================================
void readSensors() {
    unsigned long now = millis();

    // Read DHT11 Temperature & Humidity (Every >= 2.5 seconds)
    if (now - lastDhtReadTime >= Config::DHT_READ_INTERVAL_MS) {
        lastDhtReadTime = now;

        float t = dhtSensor.readTemperature();
        float h = dhtSensor.readHumidity();

        if (isnan(t) || isnan(h)) {
            sensorValid = false;
            Serial.println("[DHT11 WARN] Failed to read sensor! Check wiring and pull-up resistor.");
        } else {
            sensorValid = true;
            currentTemperature = t;
            currentHumidity    = h;
        }
    }

    // Read HW-416-B PIR Motion Sensor (Instant Edge Detection)
    bool rawMotion = (digitalRead(Pins::PIR_PIN) == HIGH);
    if (rawMotion != currentMotionState) {
        currentMotionState = rawMotion;
        const char* motionMsg = currentMotionState ? "MOTION_DETECTED" : "CLEAR";
        
        Serial.printf("[PIR] Motion status changed -> %s\n", motionMsg);
        if (mqttClient.connected()) {
            mqttClient.publish(Topics::MOTION, motionMsg, false, 0);
        }
    }
}

// ============================================================================
// PUSH BUTTON PROCESSING (Software Debounce & Gestures)
// ============================================================================
void handleButtons() {
    updateDebouncedButton(btn1, 1);
    updateDebouncedButton(btn2, 2);

    // Simultaneous Button Press: Reset to AUTO Mode
    if (btn1.stableState == LOW && btn2.stableState == LOW) {
        if (currentMode != MODE_AUTO) {
            currentMode = MODE_AUTO;
            Serial.println("[BUTTONS] Both buttons pressed -> Reset to AUTO Mode!");
            if (mqttClient.connected()) {
                mqttClient.publish(Topics::SYSTEM_MODE, "AUTO", true, 1);
            }
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
                Serial.printf("[BUTTON %d] Pressed (Manual Action)\n", btnNumber);

                if (mqttClient.connected()) {
                    const char* topic = (btnNumber == 1) ? Topics::BUTTON1_EVENT : Topics::BUTTON2_EVENT;
                    mqttClient.publish(topic, "PRESSED", false, 0);
                }

                currentMode = MODE_MANUAL;
                if (mqttClient.connected()) {
                    mqttClient.publish(Topics::SYSTEM_MODE, "MANUAL", true, 1);
                }

                if (btnNumber == 1) {
                    int target = (servo1RoofAngle > Config::SERVO_VENT_CLOSED_ANGLE) ? 
                                 Config::SERVO_VENT_CLOSED_ANGLE : Config::SERVO_VENT_OPEN_ANGLE;
                    setRoofVent(target, "BUTTON_1");
                } else if (btnNumber == 2) {
                    int target = (servo2SideAngle > Config::SERVO_VENT_CLOSED_ANGLE) ? 
                                 Config::SERVO_VENT_CLOSED_ANGLE : Config::SERVO_VENT_OPEN_ANGLE;
                    setSideVent(target, "BUTTON_2");
                }
            } else { // Button Released
                if (mqttClient.connected()) {
                    const char* topic = (btnNumber == 1) ? Topics::BUTTON1_EVENT : Topics::BUTTON2_EVENT;
                    mqttClient.publish(topic, "RELEASED", false, 0);
                }
            }
        }
    }
    btn.lastReading = reading;
}

// ============================================================================
// AUTOMATIC CLIMATE CONTROL LOGIC
// ============================================================================
void applyControlLogic() {
    if (currentMode != MODE_AUTO || !sensorValid) {
        return;
    }

    // High Temperature Trigger (>= 28.0 °C) -> Open Vents
    if (currentTemperature >= Config::TEMP_VENT_OPEN_THRESHOLD) {
        if (servo1RoofAngle != Config::SERVO_VENT_OPEN_ANGLE) {
            Serial.printf("[AUTO LOGIC] Temp (%.1f C) >= %.1f C -> Opening Roof Vent\n", 
                          currentTemperature, Config::TEMP_VENT_OPEN_THRESHOLD);
            setRoofVent(Config::SERVO_VENT_OPEN_ANGLE, "AUTO_TEMP_HIGH");
        }
        if (servo2SideAngle != Config::SERVO_VENT_OPEN_ANGLE) {
            Serial.printf("[AUTO LOGIC] Temp (%.1f C) >= %.1f C -> Opening Side Vent\n", 
                          currentTemperature, Config::TEMP_VENT_OPEN_THRESHOLD);
            setSideVent(Config::SERVO_VENT_OPEN_ANGLE, "AUTO_TEMP_HIGH");
        }
    }
    // Low Temperature Trigger (<= 24.0 °C) -> Close Vents
    else if (currentTemperature <= Config::TEMP_VENT_CLOSE_THRESHOLD) {
        if (currentHumidity >= Config::HUMIDITY_HIGH_THRESHOLD) {
            if (servo1RoofAngle != Config::SERVO_VENT_HALF_ANGLE) {
                Serial.printf("[AUTO LOGIC] High Humidity (%.1f%%) -> Half-Opening Roof Vent\n", currentHumidity);
                setRoofVent(Config::SERVO_VENT_HALF_ANGLE, "AUTO_HUMIDITY_HIGH");
            }
        } else {
            if (servo1RoofAngle != Config::SERVO_VENT_CLOSED_ANGLE) {
                Serial.printf("[AUTO LOGIC] Temp (%.1f C) <= %.1f C -> Closing Roof Vent\n", 
                              currentTemperature, Config::TEMP_VENT_CLOSE_THRESHOLD);
                setRoofVent(Config::SERVO_VENT_CLOSED_ANGLE, "AUTO_TEMP_LOW");
            }
            if (servo2SideAngle != Config::SERVO_VENT_CLOSED_ANGLE) {
                Serial.printf("[AUTO LOGIC] Temp (%.1f C) <= %.1f C -> Closing Side Vent\n", 
                              currentTemperature, Config::TEMP_VENT_CLOSE_THRESHOLD);
                setSideVent(Config::SERVO_VENT_CLOSED_ANGLE, "AUTO_TEMP_LOW");
            }
        }
    }
}

// ============================================================================
// DIRECT ACTUATOR CONTROL (SERVO 1 & SERVO 2)
// ============================================================================
void setRoofVent(int angle, const char* source) {
    angle = constrain(angle, 0, 180);
    servo1RoofAngle = angle;
    servoRoof.write(servo1RoofAngle);

    Serial.printf("[SERVO 1] Roof Vent set to %d deg [Trigger: %s]\n", servo1RoofAngle, source);

    if (mqttClient.connected()) {
        const char* stateStr = (servo1RoofAngle > 0) ? "OPEN" : "CLOSED";
        mqttClient.publish(Topics::SERVO1_STATUS, stateStr, true, 1);
    }
}

void setSideVent(int angle, const char* source) {
    angle = constrain(angle, 0, 180);
    servo2SideAngle = angle;
    servoSide.write(servo2SideAngle);

    Serial.printf("[SERVO 2] Side Vent set to %d deg [Trigger: %s]\n", servo2SideAngle, source);

    if (mqttClient.connected()) {
        const char* stateStr = (servo2SideAngle > 0) ? "OPEN" : "CLOSED";
        mqttClient.publish(Topics::SERVO2_STATUS, stateStr, true, 1);
    }
}

// ============================================================================
// PERIODIC TELEMETRY STREAMING
// ============================================================================
void publishTelemetry() {
    unsigned long now = millis();
    if (now - lastTelemetryPubTime < Config::TELEMETRY_PUBLISH_MS) {
        return;
    }
    lastTelemetryPubTime = now;

    if (!mqttClient.connected()) {
        return;
    }

    // 1. Publish Scalar Topics
    if (sensorValid) {
        mqttClient.publish(Topics::TEMPERATURE, String(currentTemperature, 1).c_str(), false, 0);
        mqttClient.publish(Topics::HUMIDITY, String(currentHumidity, 1).c_str(), false, 0);
    }

    // 2. Publish Structured JSON Payload
    JsonDocument doc;
    doc["device_id"]      = MQTT_CLIENT_ID;
    doc["uptime_sec"]     = millis() / 1000;
    doc["mode"]           = (currentMode == MODE_AUTO) ? "AUTO" : "MANUAL";
    
    JsonObject climate = doc["climate"].to<JsonObject>();
    if (sensorValid) {
        climate["temperature"] = round(currentTemperature * 10.0) / 10.0;
        climate["humidity"]    = round(currentHumidity * 10.0) / 10.0;
        climate["status"]      = "OK";
    } else {
        climate["temperature"] = nullptr;
        climate["humidity"]    = nullptr;
        climate["status"]      = "SENSOR_ERROR";
    }

    JsonObject security = doc["activity"].to<JsonObject>();
    security["motion_detected"] = currentMotionState;

    JsonObject actuators = doc["vents"].to<JsonObject>();
    actuators["roof_angle"]  = servo1RoofAngle;
    actuators["roof_state"]  = (servo1RoofAngle > 0) ? "OPEN" : "CLOSED";
    actuators["side_angle"]  = servo2SideAngle;
    actuators["side_state"]  = (servo2SideAngle > 0) ? "OPEN" : "CLOSED";

    JsonObject diagnostics = doc["diagnostics"].to<JsonObject>();
    diagnostics["wifi_rssi_dbm"] = WiFi.RSSI();
    diagnostics["free_heap_bytes"] = ESP.getFreeHeap();

    char jsonBuffer[512];
    serializeJson(doc, jsonBuffer);

    mqttClient.publish(Topics::TELEMETRY_JSON, jsonBuffer, false, 0);
}
