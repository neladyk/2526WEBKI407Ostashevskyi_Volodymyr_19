#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

using namespace httpsserver;

// ===== Параметри Wi-Fi =====
const char* ssid = "NetPolyNet";
const char* password = "passwrod2";

// ===== Сертифікат та сервер =====
SSLCert cert;
HTTPSServer httpsSrv(&cert);

// ===== Обробник головної сторінки =====
void handleHome(HTTPRequest * req, HTTPResponse * res) {
    // Basic Auth: ostashevsky:secure -> b3N0YXNoZXZza3k6c2VjdXJl
    if(req->getHeader("Authorization") != "Basic b3N0YXNoZXZza3k6c2VjdXJl") {
        res->setStatusCode(401);
        res->setHeader("WWW-Authenticate", "Basic realm=\"ESP32-Admin\"");
        res->print("Доступ заборонено!");
        return;
    }

    // Відправка HTML з LittleFS
    if(LittleFS.exists("/index.html")) {
        File file = LittleFS.open("/index.html", "r");
        res->setHeader("Content-Type", "text/html; charset=utf-8");
        res->print(file);
        file.close();
    } else {
        res->setStatusCode(404);
        res->print("Файл index.html не знайдено в LittleFS!");
    }
}

void setup() {
    Serial.begin(115200);

    // Монтування LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("Помилка монтовання LittleFS!");
        return;
    }

    // Підключення до Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Підключення до Wi-Fi");
    while(WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nIP ESP32: " + WiFi.localIP().toString());

    // Генерація самопідписаного сертифіката
    if(createSelfSignedCert(cert, KEYSIZE_2048, "CN=esp32.local,O=IoTLab,C=UA") != 0){
        Serial.println("Помилка генерації сертифіката!");
        return;
    }

    // Реєстрація маршруту
    ResourceNode* homeNode = new ResourceNode("/", "GET", &handleHome);
    httpsSrv.registerNode(homeNode);

    // Запуск HTTPS сервера
    httpsSrv.start();
    if(httpsSrv.isRunning()){
        Serial.println("HTTPS сервер запущено на порту 443");
    }
}

void loop() {
    httpsSrv.loop();
}