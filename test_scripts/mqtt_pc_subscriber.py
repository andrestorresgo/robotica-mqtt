#!/usr/bin/env python3
"""
Python MQTT Telemetry Monitor (PC)
Displays live greenhouse sensor readings, actuator states, and QoS info.
"""

import sys
import time

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[!] 'paho-mqtt' library is not installed.")
    print("    Install it by running: pip install paho-mqtt")
    sys.exit(1)

# MQTT Broker Configuration
BROKER_IP = "127.0.0.1"
BROKER_PORT = 1883
TOPIC_WILDCARD = "greenhouse/#"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[+] Connected successfully to Mosquitto Broker ({BROKER_IP}:{BROKER_PORT})")
        print(f"[+] Subscribed to topic pattern: {TOPIC_WILDCARD}")
        client.subscribe(TOPIC_WILDCARD, qos=1)
        print("-" * 75)
    else:
        print(f"[!] Connection error. Return code: {rc}")

def on_message(client, userdata, msg):
    timestamp = time.strftime("%H:%M:%S")
    payload = msg.payload.decode('utf-8', errors='ignore')
    qos = msg.qos
    retain = msg.retain
    
    print(f"[{timestamp}] TOPIC: {msg.topic}")
    print(f"         └─ Payload: {payload}")
    print(f"         └─ Info:    QoS={qos} | Retain={retain}")
    print("-" * 75)

def main():
    print("=======================================================================")
    print("      GREENHOUSE CLIMATE CONTROL - PYTHON MQTT LIVE MONITOR           ")
    print("=======================================================================")
    
    client = mqtt.Client(client_id="PC_Python_Greenhouse_Subscriber")
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER_IP, BROKER_PORT, keepalive=60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[*] Monitoring terminated by user.")
    except Exception as e:
        print(f"\n[!] Error connecting to broker: {e}")

if __name__ == "__main__":
    main()
