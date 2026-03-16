#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ===== Параметри Wi-Fi =====
const char* ssid = "NetPolyNet";
const char* password = "password";

// ===== Ультразвуковий датчик HC-SR04 =====
const int trigPin = 5;
const int echoPin = 18;

// ===== Сервери =====
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// ===== Змінна для зберігання відстані =====
float distanceCM = 0;

// ===== Функція зчитування HC-SR04 =====
float readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  float dist = duration * 0.034 / 2; // см
  return dist;
}

// ===== Відправка даних WebSocket всім клієнтам =====
void sendDistance() {
  distanceCM = readDistance();
  char buffer[64];
  sprintf(buffer, "{\"distance\": %.2f}", distanceCM);
  webSocket.broadcastTXT(buffer);
}

// ===== Обробка головної сторінки =====
void handleRoot() {
  if (LittleFS.exists("/index.html")) {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "index.html not found!");
  }
}

// ===== WebSocket події =====
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // Тут поки що нічого не робимо
}

void setup() {
  Serial.begin(115200);

  // ===== Ініціалізація LittleFS =====
  if(!LittleFS.begin(true)){
    Serial.println("LittleFS mount failed!");
    return;
  }

  // ===== Ініціалізація пінів датчика =====
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // ===== Підключення до Wi-Fi =====
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");
  Serial.println("IP: " + WiFi.localIP().toString());

  // ===== HTTP маршрут =====
  server.on("/", HTTP_GET, handleRoot);
  server.begin();

  // ===== WebSocket =====
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  server.handleClient();
  webSocket.loop();

  // Відправка даних кожні 500 мс
  static unsigned long lastSend = 0;
  if(millis() - lastSend > 500){
    sendDistance();
    lastSend = millis();
  }
}