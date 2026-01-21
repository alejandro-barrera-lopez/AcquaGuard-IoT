#include <WiFi.h>
#include <PubSubClient.h>
#include <ESP32Servo.h> // Librería para el control del servo en ESP32
#include <math.h>         // Para la función log() en el cálculo de temperatura

// --- CONFIGURACIÓN DE RED ---
const char* ssid = "ssid";
const char* password = "pass";

// --- CONFIGURACIÓN DE SERVIDORES MQTT (FOG/EDGE) ---
// El nodo intentará conectar primero con el servidor Fog (primario).
// Si falla, intentará con el servidor Edge (secundario).
const char* mqtt_servers[] = {"10.28.219.47", "10.28.219.97"}; // 0: Fog, 1: Edge
const int num_servers = 2;
const int   mqtt_port   = 1883;

// --- CONFIGURACIÓN DE TOPICS Y CLIENT ID ---
const char* mqtt_topic_pub = "acqua-guard/alex/temperatura";
const char* mqtt_topic_sub = "acqua-guard/alex/actuador";
const char* mqtt_client_id = "esp32s3-alex-node";

// --- CONFIGURACIÓN DE PINES ---
const int TEMP_SENSOR_PIN = 4;  // Pin para el sensor de temperatura analógico (GPIO 4 / A0)
const int SERVO_PIN = 3;        // Pin para el servo motor (GPIO 3 / D2)

// --- CONFIGURACIÓN DEL SENSOR DE TEMPERATURA (NTC) - Wiki Keyestudio
const float B_COEFFICIENT = 3950.0;         // Coeficiente B del termistor
const float SERIES_RESISTOR = 4700.0;       // Resistencia en serie en el módulo (4.7kΩ)
const float NOMINAL_RESISTANCE = 10000.0;   // Resistencia nominal del termistor a 25°C (10kΩ)
const float NOMINAL_TEMP_KELVIN = 298.15;   // Temperatura nominal (25°C) en Kelvin

// --- VARIABLES GLOBALES ---
WiFiClient espClient;
PubSubClient client(espClient);
Servo myServo; // Objeto para controlar el servo
unsigned long lastSend = 0;

// --- DECLARACIÓN DE FUNCIONES ---
void setup_wifi();
void connectToMqttBroker();
void callback(char* topic, byte* payload, unsigned int length);
void enviarLecturaSensor();

void setup() {
    Serial.begin(115200);

    // Configuración del servo
    myServo.attach(SERVO_PIN);
    myServo.write(0); // Posición inicial del servo
    Serial.println("Servo inicializado en 0 grados.");

    // Configuración del sensor
    pinMode(TEMP_SENSOR_PIN, INPUT);

    // Conexión a la red y al broker
    setup_wifi();
    client.setCallback(callback);
    connectToMqttBroker();
}

void loop() {
    if (!client.connected()) {
        Serial.println("Conexión MQTT perdida. Reconectando...");
        connectToMqttBroker();
    }
    client.loop();

    // Enviar telemetría cada 5 segundos
    if (millis() - lastSend > 5000) {
        lastSend = millis();
        enviarLecturaSensor();
    }
}

void connectToMqttBroker() {
    int current_server_index = 0;
    while (!client.connected()) {
        const char* current_server = mqtt_servers[current_server_index];
        client.setServer(current_server, mqtt_port);
        Serial.printf("Intentando conexión MQTT con el servidor: %s\n", current_server);

        if (client.connect(mqtt_client_id)) {
            Serial.println("¡Conectado!");
            client.subscribe(mqtt_topic_sub);
            Serial.printf("Suscrito a: %s\n", mqtt_topic_sub);
            break;
        } else {
            Serial.printf("Fallo en la conexión, rc=%d.\n", client.state());

            current_server_index++;
            if (current_server_index >= num_servers) {
                current_server_index = 0;
                Serial.println("Se intentaron todos los servidores. Esperando 5 segundos...");
                delay(5000);
            } else {
                 Serial.println("Probando con el siguiente servidor...");
            }
        }
    }
}

void setup_wifi() {
    delay(10);
    WiFi.setHostname("nodo-alex"); // Establecer el hostname del dispositivo
    Serial.println();
    Serial.print("Conectando a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectada!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) { message += (char)payload[i]; }
    Serial.printf("Mensaje recibido [%s]: %s\n", topic, message.c_str());

    if (String(topic) == mqtt_topic_sub) {

        const int ANGLE_CW = 90;   // Posición para "Clockwise"
        const int ANGLE_CCW = 0;    // Posición para "Counter-Clockwise"
        const int ANGLE_STOP = 45;  // Posición neutral para "Stop"

        if (message == "CW") {
            myServo.write(ANGLE_CW);
            Serial.printf("Comando 'CW' recibido. Servo a %d grados.\n", ANGLE_CW);
        } else if (message == "CCW") {
            myServo.write(ANGLE_CCW);
            Serial.printf("Comando 'CCW' recibido. Servo a %d grados.\n", ANGLE_CCW);
        } else if (message == "STOP") {
            myServo.write(ANGLE_STOP);
            Serial.printf("Comando 'STOP' recibido. Servo a %d grados.\n", ANGLE_STOP);
        } else {
            Serial.println("Comando no reconocido. Se esperan 'CW', 'CCW', o 'STOP'.");
        }
    }
}

void enviarLecturaSensor() {
    // Leer el valor analógico del sensor
    int adcVal = analogRead(TEMP_SENSOR_PIN);

    // Calcular la resistencia del termistor usando la fórmula y constantes de la wiki de Keyestudio
    // La fórmula R = (V_in - V_out) / V_out * R_series se simplifica para ADC a:
    float R = (4095.0 / adcVal - 1.0) * SERIES_RESISTOR;

    // Calcular la temperatura usando la ecuación del parámetro B
    float logR = log(R / NOMINAL_RESISTANCE);
    float T_inv = (1.0 / NOMINAL_TEMP_KELVIN) + (1.0 / B_COEFFICIENT) * logR;
    float temp_kelvin = 1.0 / T_inv;
    float temp_celsius = temp_kelvin - 273.15;

    Serial.printf("Sensor de Temperatura: Raw ADC=%d, Resistencia=%.2fΩ, Temp=%.2f°C\n", adcVal, R, temp_celsius);

    // Publicar el valor de temperatura
    char payload[8];
    dtostrf(temp_celsius, 4, 2, payload); // Formatear el float a string
    client.publish(mqtt_topic_pub, payload);
    Serial.printf("Valor de sensor enviado [%s]: %s °C\n", mqtt_topic_pub, payload);
}
