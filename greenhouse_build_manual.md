# Greenhouse Climate-Control Simulation: Complete Physical Build & Wiring Manual
**Platform**: ESP32 Dev Module (Standalone Controller)  
**Actuators**: 2 × SG90 Micro-Servos (Direct Hardware PWM)  
**Sensors**: DHT11 (Climate), HW-416-B PIR (Presence), & 10kΩ Potentiometer (Analog Dial / Soil Moisture)  
**Power Topology**: Dual-Power (USB for ESP32 & Logic + External 5V for Servos with Mandatory Common Ground)

---

## 1. System Architecture Overview

```
   [ Computer / Laptop ]
             |
        (USB Cable)
             v
       +-----------+          GPIO 34 (Analog In) <---------- [ 10kΩ Potentiometer Wiper ]
       |   ESP32   |          GPIO 18 (PWM Sig 1) ----------> [ SG90 Servo 1 (Roof Vent) ]
       | DEVKIT V1 |          GPIO 19 (PWM Sig 2) ----------> [ SG90 Servo 2 (Side Vent) ]
       |           |          GPIO 4  (Bidirectional) ------> [ DHT11 Climate Sensor ]
       |           |          GPIO 14 (Digital Input) ------> [ HW-416-B PIR Sensor ]
       |           |          GPIO 26 (Input Pullup) -------> [ Button 1 (Roof Override) ]
       |           |          GPIO 27 (Input Pullup) -------> [ Button 2 (Side Override) ]
       |    3V3    | ---------------------------------------> [ Potentiometer VCC Leg ]
       |    GND    | ----+
       +-----------+     |
                         | <--- MANDATORY COMMON GROUND WIRE (Connects to Blue Rail)
                         v
+---------------------------------------------------------------------------------------+
| (-) BREADBOARD BLUE RAIL: COMMON GROUND (GND)                                         |
+---------------------------------------------------------------------------------------+
| (+) BREADBOARD RED RAIL:  +5V FROM EXTERNAL POWER SUPPLY (MB102 / 5V DC Adapter)      |
+---------------------------------------------------------------------------------------+
        |            |                   |                     |
        | +5V        | +5V               | +5V                 | +5V
        v            v                   v                     v
   [ Servo 1 ]  [ Servo 2 ]          [ DHT11 ]             [ PIR Sensor ]
    (Red Wire)   (Red Wire)          (VCC Pin)              (VCC Pin)
```

---

## 2. Complete Parts List & Visual Identification

| Component | Quantity | Visual Description & Identifying Notes |
| :--- | :---: | :--- |
| **ESP32 DevKit V1 (30 or 38 pin)** | 1 | Black PCB with silver metal RF shield, Micro-USB/Type-C port, and `EN` & `BOOT` buttons. |
| **Potentiometer (10 kΩ or 5 kΩ)** | 1 | 3-legged rotary dial with a shaft/knob on top. (Left/Right legs = Power/GND, Middle = Wiper Signal). |
| **SG90 Micro-Servos (9g)** | 2 | Small blue translucent plastic motors with 3-pin wires: **Brown/Black** (GND), **Red** (+5V), and **Orange/Yellow** (PWM Signal). |
| **DHT11 Climate Sensor** | 1 | Light blue plastic grid casing (3-pin PCB module with `+`, `-`, `out`, or 4-pin bare sensor). |
| **HW-416-B / HC-SR501 PIR Sensor** | 1 | Green PCB with a white dome (Fresnel lens) on top and 2 orange adjustment potentiometers. |
| **Tactile Push Buttons** | 2 | 4-pin momentary buttons (6x6 mm) with black round actuator caps. |
| **Breadboard Power Module (MB102)** | 1 | Dual-voltage power module with USB/DC barrel jack and two sets of 3.3V/5V/OFF jumper pins. |
| **5V 2A DC Power Adapter** | 1 | Wall adapter / USB-to-barrel cable to power the MB102 module. |
| **Solderless Breadboard (830 tie-points)**| 1 | Standard prototyping breadboard with center divider and long Red (+)/Blue (-) power rails. |
| **Jumper Wires (M-M / M-F)** | ~25 | Flexible insulated jumper wires (assorted colors). |
| **Resistor: 4.7 kΩ or 10 kΩ (1/4 W)** | 1 | *(Optional)* Pull-up resistor, only required if using a bare 4-pin DHT11 sensor. |

---

## 3. Required Tools
1. **USB Cable (Micro-USB or USB-C)**: Connects ESP32 to PC for flashing and live serial monitoring.
2. **Small Flathead Screwdriver**: For adjusting PIR sensor sensitivity and delay potentiometers.
3. **Multimeter (Optional)**: Useful to verify that the power supply output is exactly 5.0 V before connecting servos.

---

## 4. Critical Safety Rules & Power Topology

> [!CAUTION]
> **RULE 1: Never power servos from the ESP32 3.3V pin:**
> An SG90 micro-servo draws **500 mA to 800 mA** under sudden movement. The ESP32's internal 3.3V regulator can only deliver ~300 mA. Powering servos from 3.3V causes brownouts, reboots, or permanently burns out the regulator. Power the servos strictly from the **External 5V rail**.

> [!IMPORTANT]
> **RULE 2: Single Common Ground (Mandatory):**
> Connect an **ESP32 GND pin** directly to the breadboard **Blue (-) Ground Rail**. Without this shared ground, the PWM signals from the ESP32 will have no voltage reference, causing the servos to jitter uncontrollably or fail to move.

> [!WARNING]
> **RULE 3: Do NOT Bridge the Two +5V Rails:**
> When the ESP32 is plugged into your computer via USB, **DO NOT connect the ESP32 `VIN` (or `5V`) pin to the breadboard's Red (+) 5V rail**. Let the computer power the ESP32 via USB, and let the external supply power the servos via the breadboard rail. Connecting two separate 5V power sources together can damage your computer's USB port.

---

## 5. Step-by-Step Assembly Instructions

### Step 1: Set Up Breadboard Power Distribution
1. Plug the **MB102 Power Supply Module** into one end of your breadboard.
2. Ensure both yellow jumpers on the power supply module are set to the **5V** position (not 3.3V, not OFF).
3. Connect your 5V 2A DC adapter to the power module barrel jack.
4. The module will now energize the breadboard's **Red Rail (+5V)** and **Blue Rail (GND)**.

### Step 2: Mount the ESP32
1. Press the ESP32 firmly across the center trench of the breadboard so each pin sits in an isolated column.
2. Run a black jumper wire from one of the **GND** pins on the ESP32 to the **Blue (-) Ground Rail**.
3. **Leave the ESP32 `VIN` / `5V` pin UNCONNECTED** (power will come from your computer's USB cable).

### Step 3: Wire the Potentiometer (Analog Input)
1. Place the potentiometer into three adjacent rows on the breadboard.
2. Connect **Leg 1 (Outer leg)** $\rightarrow$ ESP32 **3V3 Pin** (3.3V analog reference).
3. Connect **Leg 2 (Opposite outer leg)** $\rightarrow$ Breadboard **Blue (-) Ground Rail**.
4. Connect **Center Wiper Leg** $\rightarrow$ ESP32 **GPIO 34** (Analog input wire).

### Step 4: Wire the DHT11 Temperature & Humidity Sensor
1. If your DHT11 is a **3-pin module** (labeled `+`, `-`, `out` / `S`):
   - Pin `+` (VCC) $\rightarrow$ Breadboard **Red (+) Rail** (Red wire).
   - Pin `-` (GND) $\rightarrow$ Breadboard **Blue (-) Rail** (Black wire).
   - Pin `out` / `S` (Signal) $\rightarrow$ ESP32 **GPIO 4** (Yellow wire).
2. If your DHT11 is a **bare 4-pin sensor**:
   - Pin 1 (Leftmost, VCC) $\rightarrow$ Red (+) Rail.
   - Pin 2 (Data) $\rightarrow$ ESP32 **GPIO 4** (AND place a 4.7 kΩ–10 kΩ resistor between Pin 1 and Pin 2).
   - Pin 3 (NC) $\rightarrow$ Leave unconnected.
   - Pin 4 (Rightmost, GND) $\rightarrow$ Blue (-) Rail.

### Step 5: Wire the HW-416-B PIR Motion Sensor
1. Gently pull off the white plastic Fresnel dome to read the pin labels on the PCB (`VCC`, `OUT`, `GND`).
2. Pin `VCC` $\rightarrow$ Breadboard **Red (+) Rail** (Red wire).
3. Pin `GND` $\rightarrow$ Breadboard **Blue (-) Rail** (Black wire).
4. Pin `OUT` $\rightarrow$ ESP32 **GPIO 14** (Purple wire).

### Step 6: Wire the 2x Push Buttons
1. **Button 1 (Roof Vent Manual Override)**:
   - Insert Button 1 across the center trench.
   - Connect one pin to ESP32 **GPIO 26** (Green wire).
   - Connect the diagonally opposite pin to the **Blue (-) Ground Rail** (Black wire).
2. **Button 2 (Side Vent Manual Override)**:
   - Insert Button 2 across the center trench.
   - Connect one pin to ESP32 **GPIO 27** (Blue wire).
   - Connect the diagonally opposite pin to the **Blue (-) Ground Rail** (Black wire).
*(No external resistors needed: firmware automatically enables ESP32 internal `INPUT_PULLUP`).*

### Step 7: Wire the 2x SG90 Micro-Servos
1. **Servo 1 (Roof Vent SG90)**:
   - **Brown / Black Wire (GND)** $\rightarrow$ Breadboard **Blue (-) Ground Rail**.
   - **Red Wire (+5V Power)** $\rightarrow$ Breadboard **Red (+) 5V Rail**.
   - **Orange / Yellow Wire (PWM Signal)** $\rightarrow$ ESP32 **GPIO 18**.
2. **Servo 2 (Side Vent / Shade SG90)**:
   - **Brown / Black Wire (GND)** $\rightarrow$ Breadboard **Blue (-) Ground Rail**.
   - **Red Wire (+5V Power)** $\rightarrow$ Breadboard **Red (+) 5V Rail**.
   - **Orange / Yellow Wire (PWM Signal)** $\rightarrow$ ESP32 **GPIO 19**.

---

## 6. Complete Wiring Matrix

| Component & Pin | Connection Destination | Wire Color | Function & Notes |
| :--- | :--- | :---: | :--- |
| **External Supply (+5V)** | Breadboard Red Rail (+) | **Red** | High-current +5V bus for servos & sensors |
| **External Supply (GND)** | Breadboard Blue Rail (-) | **Black** | Shared Common Ground bus |
| **ESP32 USB Port** | Computer USB Port | **USB Cable**| Logic power (3.3V LDO), firmware upload & Serial |
| **ESP32 GND Pin** | Breadboard Blue Rail (-) | **Black** | **MANDATORY COMMON GROUND LINK** |
| **ESP32 VIN Pin** | **DISCONNECTED** | — | **DO NOT connect to Red rail when USB is plugged in** |
| **Potentiometer Leg 1** | ESP32 `3V3` Pin | **Red** | 3.3V Analog voltage reference |
| **Potentiometer Leg 2** | Breadboard Blue Rail (-) | **Black** | Ground reference |
| **Potentiometer Wiper** | ESP32 `GPIO 34` | **White** | Analog input signal (0–100%) |
| **DHT11 VCC (+)** | Breadboard Red Rail (+) | **Red** | Sensor 5V power |
| **DHT11 GND (-)** | Breadboard Blue Rail (-) | **Black** | Sensor Ground |
| **DHT11 DATA (Out)** | ESP32 `GPIO 4` | **Yellow** | Digital climate telemetry signal |
| **PIR VCC (+)** | Breadboard Red Rail (+) | **Red** | Sensor 5V power |
| **PIR GND (-)** | Breadboard Blue Rail (-) | **Black** | Sensor Ground |
| **PIR OUT (Signal)** | ESP32 `GPIO 14` | **Purple** | Motion detect signal (HIGH = Active) |
| **Button 1 Leg A** | ESP32 `GPIO 26` | **Green** | Roof Vent manual toggle |
| **Button 1 Leg B** | Breadboard Blue Rail (-) | **Black** | Ground contact on button press |
| **Button 2 Leg A** | ESP32 `GPIO 27` | **Blue** | Side Vent manual toggle |
| **Button 2 Leg B** | Breadboard Blue Rail (-) | **Black** | Ground contact on button press |
| **Servo 1 Red Wire (+)** | Breadboard Red Rail (+) | **Red** | Dedicated 5V power |
| **Servo 1 Brown/Black** | Breadboard Blue Rail (-) | **Black** | Servo Ground |
| **Servo 1 Orange Wire** | ESP32 `GPIO 18` | **Orange** | Roof Vent PWM Signal (Timer 0) |
| **Servo 2 Red Wire (+)** | Breadboard Red Rail (+) | **Red** | Dedicated 5V power |
| **Servo 2 Brown/Black** | Breadboard Blue Rail (-) | **Black** | Servo Ground |
| **Servo 2 Orange Wire** | ESP32 `GPIO 19` | **Orange** | Side Vent PWM Signal (Timer 1) |

---

## 7. First Power-On Checklist & Troubleshooting Guide

### Pre-Flight Checklist:
1. [ ] Is the black ground jumper connected from **ESP32 GND** to the **Breadboard Blue Rail**?
2. [ ] Is the **ESP32 VIN pin disconnected** from the external +5V rail while the USB cable is in use?
3. [ ] Is the potentiometer's outer leg connected to **3V3** (not 5V) and center wiper to **GPIO 34**?
4. [ ] Are both servo red wires connected to the **Breadboard Red Rail** and **NOT** to the ESP32 3.3V pin?
5. [ ] Have you entered your local Wi-Fi SSID, Password, and Broker IP in `include/secrets.h`?

---

### Diagnostic Troubleshooting Table:

| Symptom | Probable Cause | Exact Solution |
| :--- | :--- | :--- |
| **Potentiometer reading is erratic or jumps to 100% / 0%** | Loose connection or outer leg connected to 5V instead of 3.3V. | Ensure Potentiometer Leg 1 connects to ESP32 `3V3`, Leg 2 to `GND`, and Wiper to `GPIO 34`. |
| **Servos twitch/jitter erratically or don't move** | Missing Common Ground between ESP32 and external power supply. | Connect a jumper wire from an ESP32 `GND` pin to the breadboard's Blue (-) ground rail. |
| **ESP32 resets repeatedly with `Brownout detector was triggered`** | Servos are drawing power through the ESP32 rather than external supply. | Ensure servo red wires are plugged into the external supply's 5V rail. Check that the MB102 power adapter is rated for at least 5V 2A. |
| **DHT11 logs `Failed to read sensor!`** | Loose wire on `GPIO 4` or missing pull-up resistor on bare sensor. | Verify connection to `GPIO 4`. If using a 4-pin sensor, place a 4.7 kΩ–10 kΩ resistor between VCC and DATA. |
| **ESP32 status LED stays off (Wi-Fi fails to connect)** | Wrong credentials or attempting to connect to 5 GHz Wi-Fi. | ESP32 only supports **2.4 GHz** Wi-Fi networks. Verify credentials in `include/secrets.h`. |
| **MQTT Connection fails (Error code: -2)** | Mosquitto broker is not running or Windows Firewall is blocking Port 1883. | Ensure Mosquitto is active on your PC with `listener 1883 0.0.0.0` and `allow_anonymous true`. |

---

## 8. Software & Operational Guide

### 1. Build and Flash Firmware
In VS Code / PlatformIO Terminal:
```bash
# Compile and upload firmware to the ESP32
pio run -t upload

# Open Serial Monitor (115200 baud)
pio device monitor
```

### 2. Run Local Mosquitto Broker on PC
Create a file named `mosquitto.conf`:
```ini
listener 1883 0.0.0.0
allow_anonymous true
```
Run broker from terminal:
- **Linux/macOS**: `mosquitto -c mosquitto.conf -v`
- **Windows**: `"C:\Program Files\mosquitto\mosquitto.exe" -c mosquitto.conf -v`
- **Docker**: `docker run -it --rm -p 1883:1883 eclipse-mosquitto`

### 3. Test MQTT Topics
```bash
# Monitor all live greenhouse telemetries
mosquitto_sub -h localhost -p 1883 -t "greenhouse/#" -v

# Monitor only the Potentiometer dial
mosquitto_sub -h localhost -p 1883 -t "greenhouse/potentiometer" -v

# Remotely open Roof Vent (Servo 1)
mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo1/set" -m "OPEN"

# Remotely set Side Vent (Servo 2) to 45 degrees
mosquitto_pub -h localhost -p 1883 -t "greenhouse/servo2/set" -m "45"

# Restore automatic climate control
mosquitto_pub -h localhost -p 1883 -t "greenhouse/mode/set" -m "AUTO"
```
