#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// ----------- CONFIGURAÇÕES -------------
#define DHTPIN 15
#define DHTTYPE DHT22
#define BUZZER_PIN 4

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

int sistolica, diastolica;

unsigned long lastUpdate = 0;
const unsigned long intervalo = 3000;

// ----------- WIFI -------------
void conectaWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
}

// ----------- MQTT -------------
void conectaMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("EstacaoMichele")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha. Código: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void enviaMQTT(float temp, int sis, int dia) {
  char buffer[16];

  // Temperatura
  dtostrf(temp, 4, 1, buffer);
  client.publish("estacao/michele/temperatura", buffer);

  // Sistólica
  sprintf(buffer, "%d", sis);
  client.publish("estacao/michele/pressao_sistolica", buffer);

  // Diastólica
  sprintf(buffer, "%d", dia);
  client.publish("estacao/michele/pressao_diastolica", buffer);

  Serial.println("Enviou dados ao MQTT!");
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);

  conectaWiFi();
  client.setServer(mqtt_server, 1883);

  lcd.print("Estacao Michele");
  delay(1500);
  lcd.clear();
}

void loop() {
  if (!client.connected()) conectaMQTT();
  client.loop();

  unsigned long agora = millis();
  if (agora - lastUpdate >= intervalo) {
    lastUpdate = agora;

    float temp = dht.readTemperature();
    sistolica = random(110, 140);
    diastolica = random(70, 95);

    // EXIBIR NO LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temp, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Press: ");
    lcd.print(sistolica);
    lcd.print("/");
    lcd.print(diastolica);

    // ENVIAR MQTT
    enviaMQTT(temp, sistolica, diastolica);

    // BUZZER
    if (temp > 37.5 || sistolica > 130 || diastolica > 85) {
      tone(BUZZER_PIN, 1000);
      delay(300);
      noTone(BUZZER_PIN);
    }
  }
}
