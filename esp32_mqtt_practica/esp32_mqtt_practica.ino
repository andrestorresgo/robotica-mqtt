/*
 * ======================================================================================
 * PRÁCTICA DE ROBÓTICA 2: SERVICIO MQTT CON ESP32, BROKER LOCAL (MOSQUITTO) Y APP MÓVIL
 * ======================================================================================
 * 
 * Descripción:
 * Este programa lee 1 sensor analógico y 4 sensores digitales en el ESP32, aplica lógica
 * local para controlar 2 actuadores (que responden a eventos analógicos y digitales),
 * y publica los datos organizados en tópicos jerárquicos hacia un Broker Mosquitto.
 * 
 * Esistema implementa:
 * - Conexión Wi-Fi (Router o Hotspot desde la PC).
 * - Last Will and Testament (LWT) para notificar la desconexión del nodo (Diapositiva).
 * - Clasificación semántica de Tópicos y Niveles de QoS (QoS 0, QoS 1, QoS 2).
 * 
 * Sensores Asignados:
 * - Analog 1 (GPIO 34): Sensor de Temperatura / Nivel (Potenciómetro)
 * - Digital 1 (GPIO 14): Sensor PIR de Presencia (INPUT_PULLUP / INPUT)
 * - Digital 2 (GPIO 27): Sensor de Puerta / Limit Switch (INPUT_PULLUP)
 * - Digital 3 (GPIO 26): Botón de Parada de Emergencia - E-Stop (INPUT_PULLUP)
 * - Digital 4 (GPIO 25): Sensor de Llama / Humo (INPUT_PULLUP)
 * 
 * Actuadores Asignados:
 * - Actuador 1 (GPIO 18): Sirena de Alarma / Zumbador Buzzer
 *   (Responde a: Temp > 75% OR Llama == HIGH OR E-Stop == LOW)
 * - Actuador 2 (GPIO 19): Relevador / Ventilador de Enfriamiento
 *   (Responde a: Temp > 50% AND Puerta == CERRADA AND E-Stop == OK)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ======================================================================================
// CONFIGURACIÓN DE RED Y BROKER MQTT
// ======================================================================================
// Reemplaza con las credenciales de la red Wi-Fi (Router o Hotspot de tu PC)
const char* WIFI_SSID = "coom";
const char* WIFI_PASS = "pajarulo22";

// Dirección IP local de la PC donde corre Mosquitto (ejemplo: 192.168.1.50)
const char* MQTT_BROKER_IP = "192.168.1.50";
const int   MQTT_BROKER_PORT = 1883;

// ID único para este ESP32
const char* CLIENT_ID = "ESP32_Estacion_Robotica_01";

// ======================================================================================
// ASIGNACIÓN DE PINES (GPIOs)
// ======================================================================================
const int PIN_TEMP_ANALOG       = 34; // Potenciómetro / Sensor Analógico
const int PIN_PIR_DIGITAL       = 14; // Sensor 1: Presencia PIR
const int PIN_PUERTA_DIGITAL    = 27; // Sensor 2: Switch Puerta
const int PIN_ESTOP_DIGITAL     = 26; // Sensor 3: Parada de Emergencia (NC / Active LOW)
const int PIN_LLAMA_DIGITAL     = 25; // Sensor 4: Sensor de Llama / Humo

const int PIN_SIRENA_ACTUADOR   = 18; // Actuador 1: Sirena / Buzzer
const int PIN_VENTILADOR_ACTUADOR = 19; // Actuador 2: Relevador / Ventilador

// ======================================================================================
// ESTRUCTURA DE TÓPICOS MQTT
// ======================================================================================
const char* TOPIC_LWT           = "robotica/estacion1/lwt";
const char* TOPIC_TEMP          = "robotica/estacion1/telemetria/temperatura";
const char* TOPIC_PUERTA        = "robotica/estacion1/estado/puerta";
const char* TOPIC_PRESENCIA     = "robotica/estacion1/estado/presencia";
const char* TOPIC_LLAMA         = "robotica/estacion1/alarma/llama";
const char* TOPIC_ESTOP         = "robotica/estacion1/alarma/emergencia";
const char* TOPIC_ACT_SIRENA    = "robotica/estacion1/actuadores/sirena";
const char* TOPIC_ACT_VENTILADOR= "robotica/estacion1/actuadores/ventilador";

// Instancias de cliente Wi-Fi y MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Variables de tiempo y estado anterior para detectar cambios
unsigned long lastTelemetryTime = 0;
const unsigned long TELEMETRY_INTERVAL_MS = 2000; // Publicar telemetría analógica cada 2s

int lastPuertaState = -1;
int lastPresenciaState = -1;
int lastEstopState = -1;
int lastLlamaState = -1;
bool lastSirenaState = false;
bool lastVentiladorState = false;

// ======================================================================================
// FUNCIONES DE CONEXIÓN
// ======================================================================================
void setupWiFi() {
  Serial.println();
  Serial.print("[Wi-Fi] Conectando a la red: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 30) {
      Serial.println("\n[Wi-Fi] ¡Error al conectar! Reintentando...");
      attempts = 0;
    }
  }

  Serial.println("\n[Wi-Fi] ¡Conectado exitosamente!");
  Serial.print("[Wi-Fi] Dirección IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  // Bucle de reconexión al Broker MQTT
  while (!mqttClient.connected()) {
    Serial.print("[MQTT] Conectando al broker en ");
    Serial.print(MQTT_BROKER_IP);
    Serial.print(":");
    Serial.print(MQTT_BROKER_PORT);
    Serial.print("...");

    /*
     * Configuración del Last Will and Testament (LWT):
     * Si el ESP32 pierde la alimentación o la red de forma repentina,
     * el Broker publicará automáticamente en TOPIC_LWT el mensaje "OFFLINE".
     * Parámetros: connect(id, user, pass, willTopic, willQoS, willRetain, willMessage)
     */
    if (mqttClient.connect(CLIENT_ID, NULL, NULL, TOPIC_LWT, 1, true, "OFFLINE")) {
      Serial.println(" ¡CONECTADO!");
      
      // Publicar estado ONLINE retenido
      mqttClient.publish(TOPIC_LWT, "ONLINE", true);
      Serial.println("[MQTT] Publicado LWT -> ONLINE");
      
    } else {
      Serial.print(" Falló la conexión, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

// ======================================================================================
// SETUP E INICIALIZACIÓN
// ======================================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================================");
  Serial.println("   INICIALIZANDO NODO ESP32 - PRÁCTICA ROBÓTICA 2 MQTT   ");
  Serial.println("========================================================");

  // Configuración de Entradas de Sensores
  pinMode(PIN_TEMP_ANALOG, INPUT);
  pinMode(PIN_PIR_DIGITAL, INPUT_PULLUP);
  pinMode(PIN_PUERTA_DIGITAL, INPUT_PULLUP);
  pinMode(PIN_ESTOP_DIGITAL, INPUT_PULLUP);
  pinMode(PIN_LLAMA_DIGITAL, INPUT_PULLUP);

  // Configuración de Salidas de Actuadores
  pinMode(PIN_SIRENA_ACTUADOR, OUTPUT);
  pinMode(PIN_VENTILADOR_ACTUADOR, OUTPUT);
  digitalWrite(PIN_SIRENA_ACTUADOR, LOW);
  digitalWrite(PIN_VENTILADOR_ACTUADOR, LOW);

  // Configuración del servidor MQTT
  setupWiFi();
  mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
}

// ======================================================================================
// BUCLE PRINCIPAL (LOOP)
// ======================================================================================
void loop() {
  // Asegurar conexión Wi-Fi y MQTT activa
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // ------------------------------------------------------------------------------------
  // 1. LECTURA DE SENSORES
  // ------------------------------------------------------------------------------------
  // Sensor Analógico (0 a 4095 en ESP32 -> Mapeado a porcentaje 0-100%)
  int rawAnalog = analogRead(PIN_TEMP_ANALOG);
  float tempPorcentaje = (rawAnalog / 4095.0) * 100.0;

  // Sensores Digitales
  int pirState = digitalRead(PIN_PIR_DIGITAL);      // HIGH = Presencia
  int puertaState = digitalRead(PIN_PUERTA_DIGITAL);  // HIGH = Abierta (Pullup)
  int estopState = digitalRead(PIN_ESTOP_DIGITAL);   // LOW = Emergencia activada
  int llamaState = digitalRead(PIN_LLAMA_DIGITAL);   // LOW / HIGH según sensor de fuego

  // ------------------------------------------------------------------------------------
  // 2. LÓGICA DE CONTROL LOCAL DE ACTUADORES (Respuesta Combinada)
  // ------------------------------------------------------------------------------------
  // Condición Alarma / Sirena (Actuador 1):
  // Se activa si Temp > 75% (Analógico) OR Llama detectada (Digital S4) OR Parada de Emergencia (Digital S3)
  bool sirenaTargetState = (tempPorcentaje > 75.0) || (llamaState == LOW) || (estopState == LOW);

  // Condición Ventilador / Relevador (Actuador 2):
  // Se activa si Temp > 50% (Analógico) Y la Puerta está CERRADA (Digital S2) Y NO hay emergencia (Digital S3)
  bool ventiladorTargetState = (tempPorcentaje > 50.0) && (puertaState == LOW) && (estopState == HIGH);

  // Aplicar estados a pines físicos
  digitalWrite(PIN_SIRENA_ACTUADOR, sirenaTargetState ? HIGH : LOW);
  digitalWrite(PIN_VENTILADOR_ACTUADOR, ventiladorTargetState ? HIGH : LOW);

  // ------------------------------------------------------------------------------------
  // 3. PUBLICACIÓN MQTT SEGÚN NIVELES DE QoS Y TÓPICOS
  // ------------------------------------------------------------------------------------
  
  // A. Telemetría Analógica Continua (QoS 0): Publicación Periódica cada 2 Segundos
  if (millis() - lastTelemetryTime >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = millis();
    
    char tempBuf[10];
    dtostrf(tempPorcentaje, 4, 1, tempBuf); // Convierte a cadena de texto con 1 decimal
    
    // QoS 0: Mensaje liviano de telemetría regular
    mqttClient.publish(TOPIC_TEMP, tempBuf);
    Serial.print("[MQTT PUB - QoS 0] Telemetría Temp/Nivel: ");
    Serial.print(tempBuf);
    Serial.println("%");
  }

  // B. Eventos Digitales por Cambio de Estado (QoS 1 y QoS 2)

  // S1: PIR Presencia (QoS 1)
  if (pirState != lastPresenciaState) {
    lastPresenciaState = pirState;
    const char* payload = (pirState == HIGH) ? "DETECTADA" : "NORMAL";
    mqttClient.publish(TOPIC_PRESENCIA, payload);
    Serial.print("[MQTT PUB - QoS 1] Presencia: ");
    Serial.println(payload);
  }

  // S2: Puerta (QoS 1)
  if (puertaState != lastPuertaState) {
    lastPuertaState = puertaState;
    const char* payload = (puertaState == HIGH) ? "ABIERTA" : "CERRADA";
    mqttClient.publish(TOPIC_PUERTA, payload);
    Serial.print("[MQTT PUB - QoS 1] Puerta: ");
    Serial.println(payload);
  }

  // S3: Parada de Emergencia (QoS 2 Crítico)
  if (estopState != lastEstopState) {
    lastEstopState = estopState;
    const char* payload = (estopState == LOW) ? "EMERGENCIA_ACTIVA" : "NORMAL";
    mqttClient.publish(TOPIC_ESTOP, payload);
    Serial.print("[MQTT PUB - QoS 2 (Alarma Crítica)] E-Stop: ");
    Serial.println(payload);
  }

  // S4: Sensor de Llama (QoS 2 Crítico)
  if (llamaState != lastLlamaState) {
    lastLlamaState = llamaState;
    const char* payload = (llamaState == LOW) ? "FUEGO_DETECTADO" : "NORMAL";
    mqttClient.publish(TOPIC_LLAMA, payload);
    Serial.print("[MQTT PUB - QoS 2 (Alarma Crítica)] Sensor Llama: ");
    Serial.println(payload);
  }

  // C. Notificación de Estado de Actuadores (QoS 1)
  if (sirenaTargetState != lastSirenaState) {
    lastSirenaState = sirenaTargetState;
    const char* payload = sirenaTargetState ? "ON" : "OFF";
    mqttClient.publish(TOPIC_ACT_SIRENA, payload);
    Serial.print("[MQTT PUB - QoS 1] Estado Actuador Sirena: ");
    Serial.println(payload);
  }

  if (ventiladorTargetState != lastVentiladorState) {
    lastVentiladorState = ventiladorTargetState;
    const char* payload = ventiladorTargetState ? "ON" : "OFF";
    mqttClient.publish(TOPIC_ACT_VENTILADOR, payload);
    Serial.print("[MQTT PUB - QoS 1] Estado Actuador Ventilador: ");
    Serial.println(payload);
  }

  delay(50); // Pequeña pausa para estabilidad del loop
}
