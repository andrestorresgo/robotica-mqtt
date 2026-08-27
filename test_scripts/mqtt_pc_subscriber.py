#!/usr/bin/env python3
"""
Script de Monitoreo MQTT en Python (PC)
Útil para verificar la recepción de mensajes publicados por el ESP32,
sus niveles de QoS y los tópicos activos en tiempo real.
"""

import sys
import time

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[!] La librería 'paho-mqtt' no está instalada.")
    print("    Instálala ejecutando: pip install paho-mqtt")
    sys.exit(1)

# Configuración del Broker MQTT
BROKER_IP = "127.0.0.1"  # Usar IP de la PC o localhost
BROKER_PORT = 1883
TOPIC_WILDCARD = "robotica/estacion1/#"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[+] Conectado exitosamente al Broker MQTT ({BROKER_IP}:{BROKER_PORT})")
        print(f"[+] Suscribiéndose al patrón de tópicos: {TOPIC_WILDCARD}")
        client.subscribe(TOPIC_WILDCARD, qos=1)
        print("-" * 75)
    else:
        print(f"[!] Error de conexión. Código de retorno: {rc}")

def on_message(client, userdata, msg):
    timestamp = time.strftime("%H:%M:%S")
    payload = msg.payload.decode('utf-8', errors='ignore')
    qos = msg.qos
    retain = msg.retain
    
    print(f"[{timestamp}] TÓPICO: {msg.topic}")
    print(f"         └─ Payload: {payload}")
    print(f"         └─ Detalles: QoS={qos} | Retain={retain}")
    print("-" * 75)

def main():
    print("=======================================================================")
    print("         MONITOR DE MENSAJES MQTT - PRÁCTICA ROBÓTICA 2               ")
    print("=======================================================================")
    
    client = mqtt.Client(client_id="PC_Python_Subscriber")
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(BROKER_IP, BROKER_PORT, keepalive=60)
        client.loop_forever()
    except KeyboardInterrupt:
        print("\n[*] Monitoreo finalizado por el usuario.")
    except Exception as e:
        print(f"\n[!] Error conectando al broker: {e}")

if __name__ == "__main__":
    main()
