#include <WiFi.h>
#include <PubSubClient.h>


const char* ssid     = "Sebastian";
const char* password = "password";
 
const char* mqtt_server = "broker.emqx.io";
const int   mqtt_port   = 1883;
const char* mqtt_topic = "Datos_P2";
const int sensorPin = 4;



WiFiClient espClient;
PubSubClient client(espClient);
 
// Función para conectar á WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
 
  Serial.println("");
  Serial.println("WiFi conectada!");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}




// Reconecta co broker MQTT se se perde a conexión
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conectar a broker MQTT...");
 
    // Inténtase conectar indicando o ID do dispositivo
    // IMPORTANTE: este ID debe ser único!
    if (client.connect("NAPIoT-P2-Rec")) {
      Serial.println("conectado!");
 
      // Subscripción ao topic
      client.subscribe(mqtt_topic);
      Serial.println("Subscrito ao topic");
    } else {
      Serial.print("erro na conexión, erro=");
      Serial.print(client.state());
      Serial.println(" probando de novo en 5 segundos");
      delay(5000);
    }
  }
}
 
void enviarLecturaSensor() {
  int valorSensor = analogRead(sensorPin);   // Ler sensor
  char payload[16];
 
  // Convertimos o valor a texto (MQTT manda strings)
  sprintf(payload, "%d", valorSensor);
 
  // Publicamos no topic
  client.publish(mqtt_topic, payload);
 
  // Debug por porto serie
  Serial.print("Sensor Vapor enviado [");
  Serial.print(mqtt_topic);
  Serial.print("]: ");
  Serial.println(payload);
}
 
void setup() {
 
  // Configuración do porto serie
  Serial.begin(115200);
 
  // Conexión coa WiFi
  setup_wifi();
 
  // Configuración de MQTT
  client.setServer(mqtt_server, mqtt_port);
 
}
 
void loop() {
  // Verifica se o cliente está conectado
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
 
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 5000) {   // cada 5 segundos
    lastSend = millis();
    enviarLecturaSensor();
  }
}
