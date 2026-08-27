# Plan de Implementación: Práctica MQTT Robótica 2 (ESP32 + Mosquitto + App Móvil)

## Descripción General
Esta solución implementa un **Sistema de Control de Seguridad y Climatización Industrial / Invernadero** basado en el microcontrolador ESP32 comunicándose mediante el protocolo MQTT con un broker **Mosquitto** alojado en la PC. El celular actúa como cliente suscriptor que recibe la información en tiempo real a través de Wi-Fi local.

---

## 1. Selección de Componentes y Esquema de Hardware

Para cumplir con la prohibición del típico circuito "Botón + LED simples", el proyecto utiliza una temática industrial/robótica:

### Sensores (Total: 5)
1. **1x Sensor Analógico**: 
   - **Potenciómetro / LDR (GPIO 34)**: Simula lectura analógica continua de Temperatura o Nivel de Carga.
2. **4x Sensores Digitales**:
   - **S1 (GPIO 14)**: Sensor de Presencia PIR / Pulsador de Presencia (Entrada digital).
   - **S2 (GPIO 27)**: Final de Carrera / Reed Switch de Puerta Segura (Entrada digital).
   - **S3 (GPIO 26)**: Botón de Parada de Emergencia (E-Stop) (Entrada digital).
   - **S4 (GPIO 25)**: Sensor de Llama / Humo (Entrada digital).

### Actuadores (Total: 2)
1. **Actuador 1 (Zumbador Buzzer / Alarma Sonora o LED de Alarma)** (GPIO 18):
   - **Respuesta a Sensor Analógico**: Se activa si la Temperatura (Potenciómetro) supera un umbral crítico (ej. > 75%).
   - **Respuesta a Sensor Digital**: Se activa inmediatamente si se detecta Llama (S4) o se presiona la Parada de Emergencia (S3).
2. **Actuador 2 (Relevador / Servomotor / Ventilador)** (GPIO 19):
   - **Respuesta a Sensor Analógico**: Enciende el sistema de ventilación/refrigeración si la Temperatura supera el 50%.
   - **Respuesta a Sensor Digital**: Se desactiva inmediatamente por seguridad si la Puerta está abierta (S2) o Parada de Emergencia activa (S3).

---

## 2. Arquitectura de Tópicos MQTT y Niveles de QoS

Se estructuran tópicos jerárquicos con semántica clara y justificación explícita de niveles QoS:

| Tópico MQTT | Tipo de Datos | QoS | Justificación del Nivel de QoS |
| :--- | :--- | :---: | :--- |
| `robotica/estacion1/telemetria/temperatura` | Valor analógico (0-100%) | **QoS 0** | Telemetría continua de alta frecuencia (cada 2s). Perder un paquete individual no es crítico porque llegará otro inmediatamente. Minimiza consumo y latencia. |
| `robotica/estacion1/estado/puerta` | Cadena ("ABIERTA"/"CERRADA") | **QoS 1** | Cambio de estado de la puerta. Requiere confirmación (`PUBACK`) para asegurar que el broker registre el cambio, aunque pueda duplicarse en fallos de red. |
| `robotica/estacion1/estado/presencia` | Booleano ("DETECTADA"/"NORMAL") | **QoS 1** | Detección de presencia física. Requiere entrega confirmada al menos una vez. |
| `robotica/estacion1/alarma/llama` | Booleano ("FUEGO_DETECTADO"/"OK") | **QoS 2** | Evento crítico de seguridad (fuego/humo). Garantiza entrega exactamente una vez (`PUBREC`/`PUBREL`/`PUBCOMP`) sin duplicados ni pérdidas. |
| `robotica/estacion1/alarma/emergencia` | Booleano ("ESTOP_ACTIVO"/"OK") | **QoS 2** | Evento crítico de parada de emergencia. Requiere máxima fiabilidad y consistencia sin duplicados. |
| `robotica/estacion1/actuadores/sirena` | Estado ("ON"/"OFF") | **QoS 1** | Estado operativo del actuador 1. Confirma que el actuador respondió a la lógica local. |
| `robotica/estacion1/actuadores/ventilador` | Estado ("ON"/"OFF") | **QoS 1** | Estado operativo del actuador 2. |
| `robotica/estacion1/lwt` | Mensaje LWT ("OFFLINE"/"ONLINE") | **QoS 1 (Retained)** | **Last Will and Testament (LWT)**: Notifica automáticamente a los suscriptores si el ESP32 pierde la conexión Wi-Fi/Broker de forma inesperada (caso de la diapositiva). |

---

## 3. Configuración del Red y Broker Mosquitto (PC)

### Red Wi-Fi
- **Opción A (Recomendada)**: La PC crea una Zona con WiFi (Hotspot) y el ESP32 + Celular se conectan a esa red.
- **Opción B**: PC, ESP32 y Celular conectados al mismo Router Wi-Fi de la sala o laboratorio.

### Configuración del Broker (PC)
1. Instalar **Eclipse Mosquitto** en la PC.
2. Configurar el archivo `mosquitto.conf` para aceptar conexiones remotas de la red local:
   ```ini
   listener 1883 0.0.0.0
   allow_anonymous true
   ```
3. Iniciar el servicio Mosquitto y permitir el puerto `1883` en el Firewall de la PC.
4. Obtener la IP local de la PC (`ipconfig` en Windows o `ifconfig`/`ip a` en Linux).

---

## 4. Estructura del Código ESP32 (Arduino C++)

El firmware para ESP32 incluirá:
- Conexión Wi-Fi persistente con reconexión automática.
- Uso de la librería `PubSubClient` (o `AsyncMqttClient` para QoS 2 nativo completo).
- Configuración de LWT (Last Will & Testament) al conectar.
- Lectura continua de 1 sensor analógico y 4 digitales con antirrebote (debouncing).
- Lógica de control en tiempo real entre sensores y actuadores.
- Publicación organizada de eventos según los tópicos y niveles de QoS asignados.

---

## 5. Configuración del Celular (Suscriptor)

- Uso de aplicaciones MQTT estándar para Android/iOS: **MQTT Dash**, **MQTTX**, **MyMQTT** o **MQTT Board**.
- Conexión ingresando la IP local de la PC (Broker) en el puerto `1883`.
- Suscripciones recomendadas:
  - `robotica/estacion1/#` (Para ver todo el flujo).
  - `robotica/estacion1/alarma/+` (Suscripción filtrada por comodín a alertas críticas).
  - `robotica/estacion1/telemetria/temperatura` (Suscripción a telemetría).

---

## PreguntasAbiertas para el Usuario

> [!NOTE]
> 1. **Librerías/Framework**: ¿Prefieres el código en **Arduino IDE (C++)** o **MicroPython**? (Por defecto entregaremos Arduino C++ que es el más estandarizado en ESP32).
> 2. **Plataforma de Simulación o Hardware Real**: ¿Trabajarás con componentes físicos reales o con **Wokwi** (simulador en línea)? (Incluiremos diagrama de pines y compatibilidad total para ambos casos).

## Plan de Verificación

### Pruebas Automatizadas y de Compilación
- Validar la sintaxis del código ESP32 y dependencias.
- Proporcionar un script en Python (`test_subscriber.py` o `test_broker.py`) para simular la recepción y verificación de mensajes MQTT desde la PC.

### Verificación Manual
- Instrucciones paso a paso para verificar el broker Mosquitto en la PC.
- Guía para probar la desconexión inesperada del ESP32 y verificar el mensaje LWT en el celular.
