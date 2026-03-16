#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>

// ===== Налаштування сервера та DNS =====
WebServer myServer(80);
DNSServer myDNS;
Preferences wifiPrefs;

const IPAddress AP_IP(192, 168, 4, 1); // нова IP для точки доступу
const byte DNS_PORT = 53;
const char* AP_SSID = "ESP32_WiFi";

// ===== Збереження Wi-Fi даних =====
void saveWiFi() {
    String newSSID = myServer.arg("ssid");
    String newPASS = myServer.arg("pass");

    if(newSSID.length() > 0){
        wifiPrefs.begin("wifiCreds", false);
        wifiPrefs.putString("ssid", newSSID);
        wifiPrefs.putString("pass", newPASS);
        wifiPrefs.end();

        myServer.send(200, "text/html", "<h3>Налаштування збережено! ESP32 перезавантажується...</h3>");
        delay(2000);
        ESP.restart();
    }
}

// ===== Запуск точки доступу та Captive Portal =====
void launchAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(AP_SSID);

    myDNS.start(DNS_PORT, "*", AP_IP);

    myServer.on("/", HTTP_GET, []() {
        if(LittleFS.exists("/index.html")){
            File f = LittleFS.open("/index.html", "r");
            myServer.streamFile(f, "text/html");
            f.close();
        } else {
            myServer.send(404, "text/plain", "Файл index.html не знайдено!");
        }
    });

    myServer.on("/save", HTTP_POST, saveWiFi);

    // Редірект для всіх інших запитів
    myServer.onNotFound([]() {
        myServer.sendHeader("Location", String("http://") + AP_IP.toString(), true);
        myServer.send(302, "text/plain", "");
    });

    myServer.begin();
    Serial.println("Captive Portal активовано на IP: " + AP_IP.toString());
}

void setup() {
    Serial.begin(115200);

    if(!LittleFS.begin(true)){
        Serial.println("Помилка підключення LittleFS!");
        return;
    }

    // Зчитування збережених даних Wi-Fi
    wifiPrefs.begin("wifiCreds", true);
    String storedSSID = wifiPrefs.getString("ssid", "");
    String storedPASS = wifiPrefs.getString("pass", "");
    wifiPrefs.end();

    if(storedSSID != ""){
        WiFi.begin(storedSSID.c_str(), storedPASS.c_str());
        Serial.print("Підключення до Wi-Fi: "); Serial.println("NetPolyNet-2.4");

        int attempts = 0;
        while(WiFi.status() != WL_CONNECTED && attempts < 25){
            delay(500);
            Serial.print(".");
            attempts++;
        }
    }

    if(WiFi.status() != WL_CONNECTED){
        launchAP();
    } else {
        Serial.println("Wi-Fi підключено! IP: " + WiFi.localIP().toString());
    }
}

void loop() {
    if(WiFi.getMode() == WIFI_AP){
        myDNS.processNextRequest();
    }
    myServer.handleClient();
}