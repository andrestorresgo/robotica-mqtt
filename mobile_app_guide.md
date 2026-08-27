# Mobile App Setup & MQTT Dashboard Configuration Guide

This guide walks you through connecting your smartphone (Android or iOS) to the Mosquitto MQTT broker running on your Arch Linux computer (`192.168.1.44:1883`) and creating a dashboard to monitor sensors, analog inputs, and control the greenhouse vents.

---

## 1. Recommended Free Mobile Apps

### For Android:
- **MQTT Dash** *(Best for visual dials, buttons, gauges, and switches)*
- **IoT MQTT Panel** *(Modern dashboard interface)*
- **MQTTX Mobile** *(Clean log/chat style view)*

### For iOS (iPhone / iPad):
- **EasyMQTT** *(Visual dashboards with gauges and toggles)*
- **MQTTool** or **MQTTX** *(Streamlined topic subscriber & publisher)*

---

## 2. Connect Your Phone to the Broker

1. **Verify Wi-Fi**: Make sure your phone is connected to your **`TorresGonzalez`** Wi-Fi network (the same network your Arch Linux PC is connected to).
2. **Create New Connection in App**:
   - **Name**: `Greenhouse IoT`
   - **Broker / Host IP**: `192.168.1.44`
   - **Port**: `1883`
   - **Client ID**: `Phone_Greenhouse_Client` *(or tap random/auto)*
   - **Username & Password**: Leave empty (anonymous access is enabled)
   - **SSL/TLS**: Disabled / Off
3. Tap **Connect / Save**.
4. You should see a green dot or **"Connected"** status.

---

## 3. Creating Mobile Dashboard Widgets (Step-by-Step)

Add the following widgets to your mobile dashboard:

```
+-----------------------------------------------------------+
|                  GREENHOUSE CLIMATE PANEL                 |
+-----------------------------+-----------------------------+
|      [ Temperature ]        |        [ Humidity ]         |
|          27.4 °C            |           64.2 %            |
+-----------------------------+-----------------------------+
|    [ Potentiometer Dial ]   |      [ PIR Activity ]       |
|            45 %             |          ● CLEAR            |
+-----------------------------+-----------------------------+
|    [ Roof Vent (Servo 1) ]  |    [ Side Vent (Servo 2) ]  |
|         [ OPEN / CLOSE ]    |         [ OPEN / CLOSE ]    |
+-----------------------------+-----------------------------+
|                   [ Climate Mode ]                        |
|                    AUTO  |  MANUAL                        |
+-----------------------------------------------------------+
```

### Widget 1: Live Temperature Gauge / Text
- **Type**: `Gauge` or `Value Display`
- **Name**: `Temperature`
- **Topic**: `greenhouse/temperature`
- **Unit**: `°C`
- **Range / Format**: `0 to 50`

### Widget 2: Live Humidity Gauge / Text
- **Type**: `Gauge` or `Value Display`
- **Name**: `Humidity`
- **Topic**: `greenhouse/humidity`
- **Unit**: `%`
- **Range / Format**: `0 to 100`

### Widget 3: Potentiometer Dial / Soil Moisture Simulation
- **Type**: `Linear Gauge` or `Radial Dial / Value`
- **Name**: `Potentiometer Dial`
- **Topic**: `greenhouse/potentiometer`
- **Unit**: `%`
- **Range**: `0 to 100`

### Widget 4: PIR Motion Alarm / Indicator
- **Type**: `Indicator / LED / Text`
- **Name**: `PIR Activity`
- **Topic**: `greenhouse/motion`
- **Active / Alarm Value**: `MOTION_DETECTED` (Color: Red)
- **Normal / Idle Value**: `CLEAR` (Color: Green)

### Widget 5: Roof Vent Control Switch (Servo 1)
- **Type**: `Switch / Toggle`
- **Name**: `Roof Vent`
- **Topic to Subscribe (State)**: `greenhouse/servo1/status`
- **Topic to Publish (Command)**: `greenhouse/servo1/set`
- **ON Payload**: `OPEN`
- **OFF Payload**: `CLOSE`

### Widget 6: Side Vent Control Switch (Servo 2)
- **Type**: `Switch / Toggle`
- **Name**: `Side Vent / Shade`
- **Topic to Subscribe (State)**: `greenhouse/servo2/status`
- **Topic to Publish (Command)**: `greenhouse/servo2/set`
- **ON Payload**: `OPEN`
- **OFF Payload**: `CLOSE`

### Widget 7: Control Mode Selector (Auto vs Manual)
- **Type**: `Switch / Segmented Buttons`
- **Name**: `Climate Mode`
- **Topic to Subscribe (State)**: `greenhouse/mode/status`
- **Topic to Publish (Command)**: `greenhouse/mode/set`
- **Value 1**: `AUTO` (Greenhouse automatically opens vents when Temp $\ge 28^\circ\text{C}$)
- **Value 2**: `MANUAL` (Vents remain in user-commanded positions)

### Widget 8: ESP32 Online / Offline Status (LWT)
- **Type**: `Text / Status Indicator`
- **Name**: `ESP32 Node Status`
- **Topic**: `greenhouse/status`
- **Values**: `online` (Green) / `offline` (Red)

---

## 4. Demonstrating Key Features in Class / Defense

### Demonstration A: Live Analog & Digital Telemetry
1. Rotate the **Potentiometer knob** on the breadboard $\rightarrow$ observe the **Potentiometer Dial** widget on your phone update smoothly from $0\%$ to $100\%$ in real time.
2. Hold your finger over the **DHT11 sensor** to raise the temperature $\ge 28^\circ\text{C}$.
3. In `AUTO` mode, observe the live temperature increase on your phone, and watch both servos immediately rotate to $90^\circ$ (Open).
4. Tap the **Roof Vent Switch** on your phone to send `CLOSE`. Observe the system switch automatically to `MANUAL` mode while the roof vent closes.
5. Press both physical push buttons on the breadboard simultaneously to reset the system back to `AUTO`.

### Demonstration B: Testing Last Will and Testament (LWT)
1. On your phone, observe Widget 8 displaying `online`.
2. **Unplug the ESP32 USB cable** (simulating sudden power failure or hardware disconnect).
3. Within seconds, the Mosquitto broker detects the broken TCP connection and automatically delivers the Last Will message `offline` to `greenhouse/status`.
4. The mobile dashboard immediately turns red and indicates `offline`.
