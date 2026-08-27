#ifndef SECRETS_H
#define SECRETS_H

// ============================================================================
// WI-FI & MQTT BROKER CREDENTIALS
// ============================================================================
// Replace the placeholder values below with your real network and broker info.
// NEVER commit your real private Wi-Fi passwords to public repositories.
// ============================================================================

// 1. Wi-Fi Access Point Configuration
#define WIFI_SSID "TorresGonzalez"
#define WIFI_PASSWORD "mimia_21"

// 2. Local MQTT Broker Configuration (Computer IP running Mosquitto)
#define MQTT_BROKER_IP "192.168.1.44"
#define MQTT_BROKER_PORT 1883

// 3. Optional Broker Credentials (leave empty if allow_anonymous is true)
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// 4. MQTT Client Identity
#define MQTT_CLIENT_ID "ESP32_Greenhouse_Station_01"

#endif // SECRETS_H
