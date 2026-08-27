# Greenhouse Climate Control Simulation - Hardware Wiring & Architecture Guide

## Single Controller Architecture Overview

- **ESP32 (Standalone Master Controller & IoT Gateway)**:
  - Reads DHT11 (Temperature & Humidity) and HW-416-B PIR motion sensor.
  - Monitors manual override push buttons (Button 1 & Button 2).
  - Connects to Wi-Fi and the local Mosquitto MQTT broker.
  - Executes local automated climate control algorithms (Hysteresis & Dehumidification).
  - Directly drives **both** SG90 Micro-Servos (Roof Vent on `GPIO 18` and Side Vent on `GPIO 19`) via dedicated hardware PWM timers.

---

## Complete Pin Mapping Summary (ESP32 DevKit)

| Component | ESP32 GPIO | Mode / Type | Description / Notes |
| :--- | :--- | :--- | :--- |
| **DHT11 (Temp & Humidity)** | `GPIO 4` | Digital I/O | Data line (4.7kΩ–10kΩ pull-up to 3.3V/5V) |
| **HW-416-B (PIR Motion)** | `GPIO 14` | Digital Input | Direct digital out (HIGH = Motion Detected) |
| **Button 1 (Roof Vent Toggle)** | `GPIO 26` | Digital Input | Internal `INPUT_PULLUP` (Active LOW to GND) |
| **Button 2 (Side Vent Toggle)** | `GPIO 27` | Digital Input | Internal `INPUT_PULLUP` (Active LOW to GND) |
| **Servo 1 (Roof Vent SG90)** | `GPIO 18` | Hardware PWM | 50 Hz PWM control signal (500–2400 µs pulse) |
| **Servo 2 (Side Vent SG90)** | `GPIO 19` | Hardware PWM | 50 Hz PWM control signal (500–2400 µs pulse) |
| **Status Indicator LED** | `GPIO 2` | Digital Output | Built-in Blue LED (Solid = Wi-Fi/MQTT Connected) |

---

## Power Distribution Options

### Option A: USB Power for ESP32 + External 5V for Servos (RECOMMENDED FOR DEVELOPMENT)
This is the standard and most convenient way to build and debug:
1. **ESP32**: Powered directly from your computer via its **USB Cable** (allows flashing code & viewing Serial logs simultaneously).
2. **Servos (2x SG90)**: Powered from the **External 5V Power Supply** (MB102 module or 5V DC adapter).
   - Servo Red (+5V) wires connect to the External 5V Power Supply Red (+) rail.
   - Servo Brown/Black (GND) wires connect to the breadboard Blue (-) ground rail.
3. **CRITICAL COMMON GROUND RULE**:
   - Run a jumper wire from **ESP32 GND pin** to the **External Power Supply Blue (-) Ground Rail**.
   - **DO NOT** connect the ESP32 `VIN`/`5V` pin to the external power supply when the USB cable is plugged in (to avoid conflicting 5V voltage rails).

```
   [ Computer USB Port ]
            |
       (USB Cable)
            v
       +----------+         GPIO 18 (PWM Sig 1) --------> [ Servo 1 Signal (Orange) ]
       |  ESP32   |         GPIO 19 (PWM Sig 2) --------> [ Servo 2 Signal (Orange) ]
       |          |
       |   GND    | ----+
       +----------+     |
                        | (MANDATORY COMMON GROUND)
                        v
+-----------------------------------------------------------------------------------+
| (-) BLUE GROUND RAIL (GND)                                                        |
+-----------------------------------------------------------------------------------+
| (+) RED 5V RAIL (FROM EXTERNAL 5V SUPPLY ONLY)                                    |
+-----------------------------------------------------------------------------------+
       |          |
       | +5V      | +5V
       v          v
   [Servo 1]  [Servo 2]
    (+ Red)    (+ Red)
```

---

### Option B: Fully Standalone / Battery Operation (No PC USB)
When deploying the project away from your PC:
1. External 5V (+) Rail connects to **ESP32 VIN pin**, **Servo 1 Red wire**, and **Servo 2 Red wire**.
2. External GND (-) Rail connects to **ESP32 GND pin**, **Servo 1 Brown wire**, and **Servo 2 Brown wire**.
