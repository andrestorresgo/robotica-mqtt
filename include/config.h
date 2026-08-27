#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// HARDWARE PIN ASSIGNMENTS (ESP32 STANDALONE CONTROLLER)
// ============================================================================
namespace Pins {
    // Analog Sensors (ADC1)
    constexpr uint8_t POT_PIN         = 34;  // Potentiometer Analog Input (ADC1_CH6, 0-4095)
    
    // Digital Sensors
    constexpr uint8_t DHT_PIN         = 4;   // DHT11 Data Pin (requires pull-up, e.g. 4.7k-10k or internal)
    constexpr uint8_t PIR_PIN         = 14;  // HW-416-B PIR Motion Sensor Digital Output
    
    // User Input Buttons (Configured with internal INPUT_PULLUP)
    constexpr uint8_t BUTTON_1_PIN    = 26;  // Button 1: Roof Vent Manual Toggle (Active LOW)
    constexpr uint8_t BUTTON_2_PIN    = 27;  // Button 2: Side Vent Manual Toggle (Active LOW)
    
    // Actuators (Direct PWM driven via ESP32Servo)
    constexpr uint8_t SERVO_1_PIN     = 18;  // Servo 1: Roof Vent SG90 PWM
    constexpr uint8_t SERVO_2_PIN     = 19;  // Servo 2: Side Vent SG90 PWM
    
    // Status Indicator LED
    constexpr uint8_t STATUS_LED_PIN  = 2;   // Onboard Blue LED for Wi-Fi/MQTT status
}

// ============================================================================
// CLIMATE CONTROL LOGIC THRESHOLDS & TIMINGS
// ============================================================================
namespace Config {
    // Temperature Thresholds (Degrees Celsius)
    constexpr float TEMP_VENT_OPEN_THRESHOLD  = 28.0f; // Open vents if Temp >= 28.0 °C
    constexpr float TEMP_VENT_CLOSE_THRESHOLD = 24.0f; // Close vents if Temp <= 24.0 °C (Hysteresis)
    
    // Humidity Thresholds (Percentage)
    constexpr float HUMIDITY_HIGH_THRESHOLD   = 80.0f; // High humidity override trigger
    
    // Servo Angles (Degrees 0 - 180)
    constexpr int SERVO_VENT_CLOSED_ANGLE     = 0;   // 0 degrees = Completely Closed
    constexpr int SERVO_VENT_HALF_ANGLE       = 45;  // 45 degrees = Half-Open (Ventilation)
    constexpr int SERVO_VENT_OPEN_ANGLE       = 90;  // 90 degrees = Fully Open
    
    // Timing Intervals (Milliseconds)
    constexpr unsigned long DHT_READ_INTERVAL_MS       = 2500;  // DHT11 requires >= 2000 ms between reads
    constexpr unsigned long TELEMETRY_PUBLISH_MS       = 3000;  // Periodic MQTT telemetry rate
    constexpr unsigned long BUTTON_DEBOUNCE_MS         = 50;    // Switch debounce filter time
    constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000;  // Non-blocking MQTT retry interval
    constexpr int POT_PUBLISH_DELTA_PERCENT            = 3;     // Minimum % change to trigger instant publish
}

// ============================================================================
// MQTT TOPIC DEFINITIONS
// ============================================================================
namespace Topics {
    // Status & Telemetry
    constexpr const char* LWT_STATUS      = "greenhouse/status";          // "online" / "offline"
    constexpr const char* TELEMETRY_JSON  = "greenhouse/telemetry";       // Combined JSON payload
    constexpr const char* TEMPERATURE     = "greenhouse/temperature";     // Float string (°C)
    constexpr const char* HUMIDITY        = "greenhouse/humidity";        // Float string (% RH)
    constexpr const char* POTENTIOMETER   = "greenhouse/potentiometer";   // Integer string (0-100%)
    constexpr const char* MOTION          = "greenhouse/motion";          // "MOTION_DETECTED" / "CLEAR"
    
    // Sensor & Button Events
    constexpr const char* BUTTON1_EVENT   = "greenhouse/button1";         // "PRESSED" / "RELEASED"
    constexpr const char* BUTTON2_EVENT   = "greenhouse/button2";         // "PRESSED" / "RELEASED"
    
    // Actuator Feedback
    constexpr const char* SERVO1_STATUS   = "greenhouse/servo1/status";   // "OPEN" / "CLOSED"
    constexpr const char* SERVO2_STATUS   = "greenhouse/servo2/status";   // "OPEN" / "CLOSED"
    constexpr const char* SYSTEM_MODE     = "greenhouse/mode/status";     // "AUTO" / "MANUAL"
    
    // Inbound Commands (Subscribed)
    constexpr const char* CMD_SERVO1_SET  = "greenhouse/servo1/set";      // "OPEN", "CLOSE", "0"-"180"
    constexpr const char* CMD_SERVO2_SET  = "greenhouse/servo2/set";      // "OPEN", "CLOSE", "0"-"180"
    constexpr const char* CMD_MODE_SET    = "greenhouse/mode/set";        // "AUTO", "MANUAL"
}

#endif // CONFIG_H
