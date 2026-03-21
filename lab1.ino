#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// ===== Налаштування Wi-Fi =====
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

WebServer server(80);

// ===== Відправка HTML-файлу з LittleFS =====
void handleRoot() {
  if(!LittleFS.exists("/dashboard.html")) {
    server.send(404, "text/plain", "Файл не знайдено!");
    return;
  }

  File file = LittleFS.open("/dashboard.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void setup() {
  Serial.begin(115200);

  // ===== Ініціалізація LittleFS =====
  if(!LittleFS.begin(true)) {
    Serial.println("Помилка LittleFS!");
    return;
  }
  Serial.println("LittleFS успішно змонтовано.");

  // ===== Підключення до Wi-Fi =====
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Підключення до Wi-Fi");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi підключено!");
  Serial.println("IP адреса ESP32: " + WiFi.localIP().toString());

  // ===== Налаштування маршруту =====
  server.on("/", HTTP_GET, handleRoot);

  // ===== Запуск сервера =====
  server.begin();
  Serial.println("HTTP сервер запущено на порті 80");
}

void loop() {
  server.handleClient();
}