#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>

// Configuración WiFi
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Configuración MQTT
const char* mqtt_server = "broker.emqx.io"; 
const int mqtt_port = 1883;
const char* topic_pub_temp = "AquaGuard/temperature"; 
const char* topic_sub_actuador = "AquaGuard/alarm"; 

// Definición de Pins (FireBeetle 2 ESP32-S3)
const int sensorPin = 4;
const int buzzerPin = 17; 

WiFiClient espClient;
PubSubClient client(espClient);

// Lectura e conversión do sensor NTC
double getTemperature() {
  int rawValue = analogRead(sensorPin);
  
  if (rawValue == 0) return -999; // Erro de lectura

  // Conversión a voltaxe (12-bit)
  double voltage = (rawValue / 4095.0) * 3.3; 
  if(voltage >= 3.3) voltage = 3.29; 
  
  // Cálculo de resistencia
  double r_ntc = 4700.0 * (3.3 - voltage) / voltage;
  
  // Ecuación Steinhart-Hart
  double tempC = 1.0 / (log(r_ntc / 10000.0) / 3950.0 + 1.0 / (25.0 + 273.15)) - 273.15;
  
  return tempC;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectada!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Callback para recepción de mensaxes MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) message += (char)payload[i];
  
  if (String(topic) == topic_sub_actuador) {
    if (message == "ON" || message == "1") {
      digitalWrite(buzzerPin, HIGH); 
      Serial.println("Alarma ON");
    } else {
      digitalWrite(buzzerPin, LOW);
      Serial.println("Alarma OFF");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando a MQTT...");
    if (client.connect("AquaGuard_Alex_Node_V1")) {
      Serial.println("Conectado!");
      client.subscribe(topic_sub_actuador);
    } else {
      Serial.print("fallo, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(sensorPin, INPUT); 
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  static unsigned long lastMsg = 0;
  unsigned long now = millis();
  
  if (now - lastMsg > 5000) {
    lastMsg = now;
    double temp = getTemperature();
    
    // Filtro básico de rangos válidos
    if (temp > -50 && temp < 100) {
      char tempString[8];
      dtostrf(temp, 1, 2, tempString);
      
      client.publish(topic_pub_temp, tempString);
      
      Serial.print("Temp enviada: ");
      Serial.println(tempString);
    } else {
      Serial.println("Erro na lectura do sensor.");
    }
  }
}
