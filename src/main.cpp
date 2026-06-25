#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

// ================= KONFIGURASI =================
const char* ssid = "NAMA_WIFI_KAMU";        
const char* password = "PASSWORD_WIFI_KAMU"; 
const char* mqtt_server = "IP_ADDRESS_PC_KAMU"; // Contoh: "192.168.1.5"

#define I2C_SDA 21
#define I2C_SCL 22
#define BME688_ADDR 0x76

Adafruit_BME680 bme; 
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

void setup_wifi() {
  Serial.print("\nKonek ke WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Terhubung! IP: " + WiFi.localIP().toString());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Konek ke MQTT Broker...");
    String clientId = "ESP32-Node-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println(" Berhasil!");
    } else {
      Serial.print(" Gagal. Coba lagi 5 detik...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi I2C dan Sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!bme.begin(BME688_ADDR)) { 
    Serial.println("BME688 tidak ditemukan! Cek kabel.");
    while (1); 
  }
  Serial.println("BME688 Siap!");
  
  // Setting default BME688
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); 

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Kirim data setiap 5 detik
  if (millis() - lastMsg > 5000) {
    lastMsg = millis();

    if (!bme.performReading()) {
      Serial.println("Gagal baca sensor!");
      return;
    }

    // Buat format JSON
    char payload[150];
    snprintf(payload, sizeof(payload), 
             "{\"temp\":%.2f, \"hum\":%.2f, \"press\":%.2f, \"gas\":%.2f}", 
             bme.temperature, bme.humidity, bme.pressure / 100.0, bme.gas_resistance / 1000.0);

    Serial.print("Kirim data: ");
    Serial.println(payload);

    // Publish ke broker
    client.publish("iiot/test/bme688", payload);
  }
}