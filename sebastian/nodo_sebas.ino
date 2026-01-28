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
const char* mqtt_topic_pub = "acqua-guard/sebastian/vapor";
const char* mqtt_topic_sub = "acqua-guard/sebastian/actuador";
const char* mqtt_client_id = "esp32s3-sebas-node";

// --- Configuración PINES ---
const int VAPOR_SENSOR_PIN = 4; // Pin para el sensor de vapor (analógico)
const int buzzerPin = 17; // Pin para el actuador (buzzer)
const int ledPin = 21; // Pin para el LED integrado (GPIO 21)

// --- Variables globales ---
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastSend = 0;

// Variables para el manejo de la alarma
bool alarmActive = false;
unsigned long lastBuzzerToggle = 0;
const long buzzerInterval = 125; // 125ms de intervalo para un toggle de 4Hz (8 cambios de estado por segundo)

// --- Declaración de funciones ---
void setup_wifi();
void connectToMqttBroker();
void callback(char* topic, byte* payload, unsigned int length);
void enviarLecturaSensor();

void setup() {
    Serial.begin(115200);
    pinMode(VAPOR_SENSOR_PIN, INPUT);
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
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

    if (millis() - lastSend > 5000) {
        lastSend = millis();
        enviarLecturaSensor();
    }

    // Si la alarma está activa, hacer sonar el buzzer de forma intermitente
    if (alarmActive) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastBuzzerToggle >= buzzerInterval) {
            lastBuzzerToggle = currentMillis;
            digitalWrite(buzzerPin, !digitalRead(buzzerPin));
        }
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
    WiFi.setHostname("nodo-sebas");
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
        if (message == "ALARM_ON") {
            alarmActive = true;
            Serial.println("Alarma ACTIVADA");
        } else if (message == "ALARM_OFF") {
            alarmActive = false;
            digitalWrite(buzzerPin, LOW); // Asegurarse de que el buzzer se apaga
            Serial.println("Alarma DESACTIVADA");
        } else if (message == "BUZZER_ON") {
            alarmActive = false; // Desactivar la alarma si se usa el control manual
            digitalWrite(buzzerPin, HIGH);
            Serial.println("Buzzer ON (manual)");
        } else if (message == "BUZZER_OFF") {
            alarmActive = false; // Desactivar la alarma si se usa el control manual
            digitalWrite(buzzerPin, LOW);
            Serial.println("Buzzer OFF (manual)");
        } else if (message == "TOGGLE_LED") {
            digitalWrite(ledPin, !digitalRead(ledPin));
            Serial.printf("LED state: %s\n", digitalRead(ledPin) ? "ON" : "OFF");
        }
    }
}

void enviarLecturaSensor() {
    int valorSensor = analogRead(VAPOR_SENSOR_PIN);
    char payload[8];
    sprintf(payload, "%d", valorSensor);
    client.publish(mqtt_topic_pub, payload);
    Serial.printf("Valor de sensor enviado [%s]: %s\n", mqtt_topic_pub, payload);
}