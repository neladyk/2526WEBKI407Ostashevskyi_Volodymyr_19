#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ===== Налаштування Wi-Fi =====
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

WebServer server(80);
const int LED_PIN = 2; // Вбудований LED
bool ledStatus = false;

// ===== GET /status - повертає JSON зі станом LED =====
void handleStatus() {
    DynamicJsonDocument doc(128);
    doc["led"] = ledStatus;
    String resp;
    serializeJson(doc, resp);
    server.send(200, "application/json", resp);
}

// ===== POST /control - керування LED =====
void handleControl() {
    if(server.hasArg("plain")) {
        String body = server.arg("plain");
        DynamicJsonDocument doc(128);
        DeserializationError err = deserializeJson(doc, body);

        if(!err) {
            String cmd = doc["command"];
            if(cmd == "on") ledStatus = true;
            else if(cmd == "off") ledStatus = false;

            digitalWrite(LED_PIN, ledStatus ? HIGH : LOW);
            server.send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            server.send(400, "application/json", "{\"error\":\"Невірний JSON\"}");
        }
    } else {
        server.send(400, "text/plain", "Відсутнє тіло запиту");
    }
}

// ===== Відправка HTML з LittleFS =====
void handleRoot() {
    if(!LittleFS.exists("/dashboard.html")) {
        server.send(404, "text/plain", "Файл dashboard.html відсутній у LittleFS.");
        return;
    }
    File file = LittleFS.open("/dashboard.html", "r");
    server.streamFile(file, "text/html");
    file.close();
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // ===== Ініціалізація LittleFS =====
    if(!LittleFS.begin(true)) {
        Serial.println("Помилка LittleFS!");
        return;
    }
    Serial.println("LittleFS змонтовано успішно.");

    // ===== Підключення до Wi-Fi =====
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Підключення до Wi-Fi");
    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWi-Fi підключено!");
    Serial.println("IP ESP32: " + WiFi.localIP().toString());

    // ===== Маршрути сервера =====
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/control", HTTP_POST, handleControl);

    server.begin();
    Serial.println("HTTP сервер запущено на порті 80");
}

void loop() {
    server.handleClient();
}