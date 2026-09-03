# Arduino IoT Cloud: Complete Step-by-Step Setup Guide
**Project**: Greenhouse Climate-Control Simulation (ESP32)  
**Cloud Tier**: Free Tier (Community Plan - 4 Cloud Variables)  
**Hardware Status**: **100% Unchanged** (Uses existing breadboard circuit and wiring)

---

## Architecture Overview

```
+-----------------------------------------------------------------------------------+
|                            ARDUINO IOT CLOUD PLATFORM                             |
|                                                                                   |
|   [ Dashboard Widgets ]                                                           |
|     - Humidity Gauge           <---+ (Read-Only Telemetry)                        |
|     - Potentiometer Dial       <---+ (Read-Only Telemetry)                        |
|     - Roof Vent Switch (SG90)  <---> (Bi-directional Command & State)             |
|     - Side Vent Switch (SG90)  <---> (Bi-directional Command & State)             |
+------------------------------------------^----------------------------------------+
                                           | (Encrypted MQTT over TLS)
                                           v
+-----------------------------------------------------------------------------------+
|                        ESP32 DEVKIT V1 (STANDALONE NODE)                          |
|                                                                                   |
|  - GPIO 34 : Potentiometer (ADC1)       --> Syncs with 'potentiometer' (0-100%)   |
|  - GPIO 4  : DHT11 Sensor               --> Syncs with 'humidity' (% RH)          |
|  - GPIO 14 : PIR Motion Sensor          --> Local Serial Monitor logging          |
|  - GPIO 26 : Button 1 (INPUT_PULLUP)    --> Toggles Servo 1 & 'roofVent' variable |
|  - GPIO 27 : Button 2 (INPUT_PULLUP)    --> Toggles Servo 2 & 'sideVent' variable |
|  - GPIO 18 : Servo 1 (Roof Vent SG90)   --> Controlled by 'roofVent' (0° / 90°)   |
|  - GPIO 19 : Servo 2 (Side Vent SG90)   --> Controlled by 'sideVent' (0° / 90°)   |
|  - GPIO 2  : Built-in Blue LED          --> Solid Blue = Connected to Cloud       |
+-----------------------------------------------------------------------------------+
```

---

## Step 1: Install the Arduino Cloud Agent

The **Arduino Cloud Agent** is a lightweight background service that enables your web browser to communicate with your ESP32 via USB.

1. Navigate to: [https://cloud.arduino.cc](https://cloud.arduino.cc) and log in (or create a free account).
2. Go to the **IoT Cloud** section: [https://create.arduino.cc/iot](https://create.arduino.cc/iot).
3. If prompted, download and install the **Arduino Create Agent** for Linux, Windows, or macOS.
4. Once installed, verify that the tray icon shows **Agent is running**.

---

## Step 2: Register your ESP32 Device

1. In the Arduino IoT Cloud menu, click on **Devices** (left sidebar).
2. Click **Add Device** $\rightarrow$ select **Third party device**.
3. Select Device Type:
   - Manufacturer: **ESP32**
   - Model: **DOIT ESP32 DEVKIT V1** (or **ESP32 Dev Module**)
4. Click **Continue**.
5. Give your device a name (e.g., `ESP32_Greenhouse_Node`).
6. **CRITICAL STEP - SAVE CREDENTIALS**:
   - Arduino Cloud will present your **Device ID** and **Secret Key**.
   - Click **Download PDF** or copy the **Secret Key** immediately to a safe place.
   - *(Note: The Secret Key cannot be retrieved later; if lost, you must generate a new one).*
7. Click **Done**. Your device will now appear in the Devices list.

---

## Step 3: Create the "Thing" & Configure Network

1. In the left navigation bar, click on **Things**.
2. Click **Create Thing** and name it `Greenhouse_Thing`.
3. In the Thing configuration screen:
   - **Associate Device**: Under the *Device* section on the right, click **Select Device** and choose your `ESP32_Greenhouse_Node`.
   - **Configure Network**: Under the *Network* section on the right, click **Configure**:
     - **Wi-Fi SSID**: Enter your **2.4 GHz Wi-Fi** network name.
     - **Password**: Enter your Wi-Fi password.
     - **Secret Key**: Paste the Secret Key you copied in Step 2.
     - Click **Save**.

---

## Step 4: Add the 4 Cloud Variables (Free Tier Limit)

Click **Add Variable** in the Thing screen and configure each of the 4 variables using the basic types available in the UI:

| # | Variable Name | UI Variable Type to Select | C++ Type | Permission | Update Policy | Callback Generated |
| :-: | :--- | :--- | :---: | :---: | :---: | :--- |
| **1** | `humidity` | **Floating Point Number** | `float` | **Read Only** | On change | *None* |
| **2** | `potentiometer` | **Integer Number** | `int` | **Read Only** | On change | *None* |
| **3** | `roofVent` | **Boolean** | `bool` | **Read & Write** | On change | `onRoofVentChange()` |
| **4** | `sideVent` | **Boolean** | `bool` | **Read & Write** | On change | `onSideVentChange()` |

> [!TIP]
> - For **`roofVent`** and **`sideVent`**, selecting **Read & Write** automatically configures the callback functions (`onRoofVentChange` and `onSideVentChange`).
> - When you create them as **Boolean**, you can link them directly to **Switch** widgets in the Dashboard!
> - When you create **`humidity`** as **Floating Point Number** and **`potentiometer`** as **Integer Number**, you can link them directly to **Gauge** and **Percentage** widgets in the Dashboard!

---

## Step 5: Open the Web Editor and Paste the Code

1. At the top of your Thing page, click on the **Sketch** tab (or click **Open full editor**).
2. You will see three files automatically prepared by Arduino Cloud:
   - `Greenhouse_Thing.ino` (Main code)
   - `thingProperties.h` (Auto-generated properties and variables)
   - `Secret` tab (`arduino_secrets.h`)
3. Open the main `.ino` tab, select all existing sample code, delete it, and paste the code from:
   [`esp32_arduino_cloud/esp32_arduino_cloud.ino`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/esp32_arduino_cloud/esp32_arduino_cloud.ino)
4. Check the `thingProperties.h` tab:
   - Arduino Cloud will have generated the properties corresponding to the 4 variables you created.
   - It matches our reference file [`esp32_arduino_cloud/thingProperties.h`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/esp32_arduino_cloud/thingProperties.h).
5. **Verify Libraries**:
   - Click on the **Libraries** icon (bookshelf icon on the left toolbar).
   - In the search bar, search and ensure the following are available:
     - `ESP32Servo` by Kevin Harrington / John K. Bennett
     - `DHT sensor library` by Adafruit
     - `Adafruit Unified Sensor` by Adafruit
   *(Note: Arduino Cloud includes these popular libraries automatically, but you can click 'Include' if needed).*

---

## Step 6: Compile and Flash to ESP32

1. Plug your ESP32 into your computer using the USB data cable.
2. In the top dropdown bar of the Web Editor, select your board port (e.g., `DOIT ESP32 DEVKIT V1` on `/dev/ttyUSB0` or `COMx`).
3. Click the **Verify** button (check-mark) to ensure zero compilation errors.
4. Click the **Upload** button (right-arrow $\rightarrow$).
5. Once upload completes:
   - Open the **Serial Monitor** (magnifying glass or monitor icon) and set the baud rate to **115200**.
   - You should see:
     ```text
     ==================================================
      GREENHOUSE CLIMATE CONTROL - ARDUINO IOT CLOUD
     ==================================================
     [DHT11] Sensor initialized on GPIO 4
     [POT] Analog potentiometer on GPIO 34
     [SERVOS] Servo 1 (Roof Vent) on GPIO 18
     [SERVOS] Servo 2 (Side Vent) on GPIO 19
     Connected to "Your_WiFi_SSID"
     Connected to Arduino IoT Cloud
     ```
   - The ESP32's built-in blue LED (`GPIO 2`) will turn **solid blue**, indicating successful connection.

---

## Step 7: Build the Cloud Dashboard

1. In the top navigation bar, click on **Dashboards**.
2. Click **Create Dashboard** and name it `Greenhouse Climate Control`.
3. Click **Add** $\rightarrow$ select **Things** $\rightarrow$ pick `Greenhouse_Thing` (or add widgets individually):

```
+-----------------------------------------------------------+
|               GREENHOUSE ARDUINO CLOUD PANEL              |
+-----------------------------+-----------------------------+
|    [ Humidity (DHT11) ]     |    [ Potentiometer Dial ]   |
|         [ Gauge ]           |           [ Dial ]          |
|          65.4 %             |             42 %            |
|     (Variable: humidity)    |  (Variable: potentiometer)  |
+-----------------------------+-----------------------------+
|   [ Roof Vent (Servo 1) ]   |   [ Side Vent (Servo 2) ]   |
|         [ Switch ]          |          [ Switch ]         |
|         ON / OFF            |          ON / OFF           |
|     (Variable: roofVent)    |     (Variable: sideVent)    |
+-----------------------------+-----------------------------+
```

### Widget Details:
1. **Widget 1: Humidity**
   - Type: **Gauge** or **Value**
   - Title: `Humidity`
   - Linked Variable: `humidity`
   - Range: `0 to 100 %`
2. **Widget 2: Potentiometer**
   - Type: **Percentage** or **Gauge**
   - Title: `Potentiometer Dial`
   - Linked Variable: `potentiometer`
   - Range: `0 to 100 %`
3. **Widget 3: Roof Vent Switch**
   - Type: **Switch**
   - Title: `Roof Vent (Servo 1)`
   - Linked Variable: `roofVent`
4. **Widget 4: Side Vent Switch**
   - Type: **Switch**
   - Title: `Side Vent (Servo 2)`
   - Linked Variable: `sideVent`

---

## Step 8: Mobile Monitoring (iOS & Android)

You can view and control the exact same dashboard from your smartphone anywhere in the world (even outside your home Wi-Fi):

1. Download **Arduino IoT Cloud Remote**:
   - **Google Play Store (Android)**: Search for *Arduino IoT Cloud Remote*.
   - **Apple App Store (iOS)**: Search for *Arduino IoT Cloud Remote*.
2. Open the app and log in with your Arduino Cloud account.
3. Select `Greenhouse Climate Control` dashboard.
4. You will see your live gauges and toggle switches on your phone screen!

---

## Step 9: Operational Testing & Verification

1. **Potentiometer Dial Test**:
   - Rotate the potentiometer knob on the breadboard.
   - The dashboard dial widget will update smoothly in near real-time.
2. **Cloud-to-Hardware Vent Control**:
   - Tap the `Roof Vent` switch on the Cloud dashboard to **ON**.
   - Servo 1 (Roof Vent on GPIO 18) rotates to **90°**.
   - Tap the switch to **OFF**. Servo 1 rotates back to **0°**.
   - Repeat with the `Side Vent` switch (Servo 2 on GPIO 19).
3. **Hardware-to-Cloud Vent Control (Physical Buttons)**:
   - Press **Button 1** on the breadboard (GPIO 26).
   - Servo 1 toggles between 0° and 90°, AND the `Roof Vent` switch on the Cloud dashboard updates its ON/OFF state automatically.
   - Press **Button 2** on the breadboard (GPIO 27).
   - Servo 2 toggles between 0° and 90°, AND the `Side Vent` switch on the Cloud dashboard updates automatically.
4. **Simultaneous Button Reset**:
   - Press both Button 1 and Button 2 simultaneously.
   - Both servos immediately close (0°) and both Cloud switches reset to **OFF**.
5. **DHT11 Humidity Test**:
   - Blow warm breath onto the DHT11 sensor.
   - The humidity reading on the dashboard will rise dynamically.
6. **PIR Sensor**:
   - Move your hand over the PIR sensor dome.
   - Notice the instant detection logged in the Serial Monitor.

---

## Summary of Completed Files in Workspace

- Firmware Sketch: [`esp32_arduino_cloud/esp32_arduino_cloud.ino`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/esp32_arduino_cloud/esp32_arduino_cloud.ino)
- Generated Properties Reference: [`esp32_arduino_cloud/thingProperties.h`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/esp32_arduino_cloud/thingProperties.h)
- Credentials Template: [`esp32_arduino_cloud/arduino_secrets.h`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/esp32_arduino_cloud/arduino_secrets.h)
- Full Setup Guide: [`arduino_cloud_setup_guide.md`](file:///mnt/Data/Dev/UNI/robotica/practicamqtt/robotica2-main/TrabajoGrupo/arduino_cloud_setup_guide.md)
