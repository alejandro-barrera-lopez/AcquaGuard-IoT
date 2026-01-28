#include <WiFi.h>
#include <PubSubClient.h>

// --- Configuración ---
const char* ssid = "ssid";
const char* password = "pass";

// --- Configuración de servidores MQTT (Fog y Edge) ---
const char* mqtt_servers[] = {"10.97.193.97", "10.97.193.47"}; // 0: Fog, 1: Edge
const int num_servers = 2;
const int   mqtt_port   = 1883;

// --- Configuración MQTT ---
const char* mqtt_topic_pub = "acqua-guard/juan-victor/luminosidad";
const char* mqtt_topic_sub = "acqua-guard/juan-victor/actuador";
const char* mqtt_client_id = "esp32s3-jv-node";

// --- Configuración pines ---
const int LUMINOSITY_SENSOR_PIN = 14; // Sensor TEMT6000 en IO14
const int redLedPin = 10;             // LED Rojo del semáforo en IO10
const int yellowLedPin = 6;           // LED Amarillo del semáforo en IO6
const int greenLedPin = 4;            // LED Verde del semáforo en IO4

// --- Variables globales ---
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastSend = 0;

// --- Declaración de funciones ---
void setup_wifi();
void connectToMqttBroker();
void callback(char* topic, byte* payload, unsigned int length);
void enviarLecturaSensor();

void setup() {
    Serial.begin(115200);

    // Configuración de pines
    pinMode(LUMINOSITY_SENSOR_PIN, INPUT);
    pinMode(redLedPin, OUTPUT);
    pinMode(yellowLedPin, OUTPUT);
    pinMode(greenLedPin, OUTPUT);

    // Asegurarse de que los LEDs estén apagados al inicio
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    digitalWrite(greenLedPin, LOW);

    setup_wifi();
    client.setCallback(callback);
    connectToMqttBroker(); // Conexión inicial
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
            break; // Salir del bucle while una vez conectado
        } else {
            Serial.printf("Fallo en la conexión, rc=%d.\n", client.state());

            current_server_index++;
            if (current_server_index >= num_servers) {
                current_server_index = 0; // Volver al servidor primario
                Serial.println("Se intentaron todos los servidores. Esperando 5 segundos para reintentar...");
                delay(5000);
            } else {
                 Serial.println("Probando con el siguiente servidor...");
            }
        }
    }
}

void setup_wifi() {
    delay(10);
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
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) { message += (char)payload[i]; }
    Serial.printf("Mensaje recibido [%s]: %s\n", topic, message.c_str());

    if (String(topic) == mqtt_topic_sub) {
        int command = message.toInt();

        // Bit 0: Green, Bit 1: Yellow, Bit 2: Red
        digitalWrite(greenLedPin, (command & 1) ? HIGH : LOW);
        digitalWrite(yellowLedPin, (command & 2) ? HIGH : LOW);
        digitalWrite(redLedPin, (command & 4) ? HIGH : LOW);

        Serial.printf("Comando de semáforo recibido: %d\n", command);
    }
}

void enviarLecturaSensor() {
    int rawValue = analogRead(LUMINOSITY_SENSOR_PIN);
    Serial.printf("Luminosidad: Raw value = %d (Pin %d)\n", rawValue, LUMINOSITY_SENSOR_PIN);

    float analogValue = (float)rawValue;      // Valor de 0 a 1023
    int percentage = map(analogValue, 0, 511, 0, 100); // Mapear el valor analógico directamente al porcentaje

    // Asegurar que el valor esté en el rango 0-100
    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    Serial.printf("Luminosidad: Calculated percentage = %d%%\n", percentage);

    char payload[8];
    sprintf(payload, "%d", percentage);
    client.publish(mqtt_topic_pub, payload);
    Serial.printf("Valor de sensor (Luminosidad %%) enviado [%s]: %s\n", mqtt_topic_pub, payload);
}
