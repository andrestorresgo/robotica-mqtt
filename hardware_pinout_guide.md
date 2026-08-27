# Greenhouse Climate Control Simulation - Hardware Wiring & Architecture Guide

## Single Controller Architecture Overview

- **ESP32 (Standalone Master Controller & IoT Gateway)**:
  - Reads **Potentiometer** analog dial (0–100%) on `GPIO 34`.
  - Reads **DHT11** (Temperature & Humidity) on `GPIO 4` and **HW-416-B PIR** motion sensor on `GPIO 14`.
  - Monitors manual override push buttons (**Button 1** on `GPIO 26` & **Button 2** on `GPIO 27`).
  - Connects to Wi-Fi and the local Mosquitto MQTT broker.
  - Executes local automated climate control algorithms (Hysteresis & Dehumidification).
  - Directly drives **both** SG90 Micro-Servos (Roof Vent on `GPIO 18` and Side Vent on `GPIO 19`) via dedicated hardware PWM timers.

---

## Complete Pin Mapping Summary (ESP32 DevKit)

| Component | ESP32 GPIO | Mode / Type | Description / Notes |
| :--- | :--- | :--- | :--- |
| **Potentiometer (Analog Dial)**| `GPIO 34` | Analog Input (ADC1) | Center wiper pin (0–3.3V $\rightarrow$ 0–4095 raw $\rightarrow$ 0–100%) |
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
1. **ESP32**: Powered directly from your computer via its **USB Cable** (allows flashing code & viewing Serial logs simultaneously).
2. **Potentiometer**:
   - Leg 1 $\rightarrow$ ESP32 `3V3` pin.
   - Leg 2 $\rightarrow$ Breadboard Blue (-) Ground rail.
   - Center Wiper Leg $\rightarrow$ ESP32 `GPIO 34`.
3. **Servos (2x SG90)**: Powered from the **External 5V Power Supply** (MB102 module or 5V DC adapter).
   - Servo Red (+5V) wires connect to the External 5V Power Supply Red (+) rail.
   - Servo Brown/Black (GND) wires connect to the breadboard Blue (-) ground rail.
4. **CRITICAL COMMON GROUND RULE**:
   - Run a jumper wire from **ESP32 GND pin** to the **External Power Supply Blue (-) Ground Rail**.
   - **DO NOT** connect the ESP32 `VIN`/`5V` pin to the external power supply when the USB cable is plugged in.

```
   [ Computer USB Port ]
            |
       (USB Cable)
            v
       +----------+         GPIO 34 (Analog In) <------- [ Potentiometer Wiper ]
       |  ESP32   |         GPIO 18 (PWM Sig 1) -------> [ Servo 1 Signal (Orange) ]
       |          |         GPIO 19 (PWM Sig 2) -------> [ Servo 2 Signal (Orange) ]
       |   3V3    | -----------------------------------> [ Potentiometer Leg 1 ]
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
