#include <WiFi.h>
#include "time.h"
#include <ESP32Servo.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ------------------- Configurar Dia e Hora -------------------
const char* diasAbrev[7] = {"DOM","SEG","TER","QUA","QUI","SEX","SAB"};
String diaAtual;
int horaAtual;
int minAtual;

// ------------------- Entradas do Flutter -------------------
const int MAX_ENTRADAS = 504;
String entradas[MAX_ENTRADAS][4];
int contadorEntradas = 0;

// ------------------- Configurações Wi-Fi -------------------
const char* ssid       = "RedmiNote13Pro";
const char* password   = "86754231";

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;  
const int   daylightOffset_sec = 3600;     

WebServer server(80);

// ------------------- Servos -------------------
Servo servo1, servo2, servo3;

bool servo1posicao = false;
bool servo2posicao = false;
bool servo3posicao = false;

// ------------------- Botão -------------------
bool delaybotao = false; 
int contador = 0; 
int tempoUltimaPressao = 0;

// ------------------- Pinos -------------------
const int relayPin = 5;
const int botao1Pin = 12;
const int botao2Pin = 13;
const int buzzerPin = 18;
const int servoPin1 = 19;
const int servoPin2 = 21;
const int servoPin3 = 22;
const int bluePin = 25;
const int greenPin = 26;
const int redPin = 27;

// ------------------- Funções CORS -------------------
void handleOptions() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

// ------------------- Função para receber JSON -------------------
void handlePost() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "JSON em falta");
    return;
  }

  String body = server.arg("plain");
  Serial.println("JSON recebido:");
  Serial.println(body);

  StaticJsonDocument<8192> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    Serial.print("Erro no JSON: ");
    Serial.println(error.c_str());
    server.send(400, "text/plain", "JSON inválido");
    return;
  }

  JsonArray dados = doc["dados"].as<JsonArray>();

  for (JsonObject item : dados) {
    if (contadorEntradas >= MAX_ENTRADAS) break; // evita overflow

    entradas[contadorEntradas][0] = item["Dia"].as<const char*>();
    entradas[contadorEntradas][1] = String(item["Hora"].as<int>());
    entradas[contadorEntradas][2] = item["Medicamento"].as<const char*>();
    entradas[contadorEntradas][3] = String(item["Quantidade"].as<int>());

    contadorEntradas++;
  }

  server.send(200, "application/json", "{\"status\":\"recebido\"}");
}

// ------------------- Setup -------------------
void setup() {
  Serial.begin(115200);

  // Pinos
  pinMode(botao1Pin, INPUT_PULLUP); pinMode(botao2Pin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT); digitalWrite(relayPin, LOW);
  pinMode(redPin, OUTPUT); pinMode(greenPin, OUTPUT); pinMode(bluePin, OUTPUT);

  // Servos
  servo1.attach(servoPin1); servo2.attach(servoPin2); servo3.attach(servoPin3);
  servo1.write(0); servo2.write(0); servo3.write(0);

  // Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.println(WiFi.localIP());

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Endpoints HTTP
  server.on("/data", HTTP_POST, handlePost);
  server.on("/data", HTTP_OPTIONS, handleOptions);
  server.begin();
}

// ------------------- Loop -------------------
void loop() {
  server.handleClient();

  // Dia e Hora Atual
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    diaAtual = diasAbrev[timeinfo.tm_wday];
    horaAtual = timeinfo.tm_hour;
    minAtual = timeinfo.tm_min;
    if (timeinfo.tm_sec == 0) { 
      Serial.printf("%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min); 
      delay(1000); 
    } 
  }

  // Programa Semanal
  for (int i = 0; i < contadorEntradas; i++) {
    if (entradas[i][0] == diaAtual && entradas[i][1].toInt() == horaAtual && minAtual == 0) {
      if (entradas[i][2] == "Ibuprofeno") {
        int qtd = entradas[i][3].toInt();
        dispensarIbuprofeno(qtd);
      } else if (entradas[i][2] == "Paracetamol") {
        int qtd = entradas[i][3].toInt();
        dispensarParacetamol(qtd);
      } else if (entradas[i][2] == "Sedatif") {
        int qtd = entradas[i][3].toInt();
        dispensarSedatif(qtd);
      }
    }
  }

  // Botão 1
  int estado = digitalRead(botao1Pin);
  if (estado == LOW && !delaybotao) { 
    contador++;
    if (contador >= 4) contador = 1;
    tempoUltimaPressao = millis();
    delaybotao = true;
  }

  switch(contador) {
    case 1: setColor(150,0,255); break;
    case 2: setColor(0,0,255); break;
    case 3: setColor(0,100,255); break;
  }

  if (estado == HIGH && delaybotao) delaybotao = false;

  if (contador > 0 && millis() - tempoUltimaPressao > 1000) {
    switch(contador) {
      case 1:
        setColor(0,0,0);
        servo1.write(servo1posicao ? 0 : 180);
        servo1posicao = !servo1posicao;
        break;
      case 2:
        setColor(0,0,0);
        servo2.write(servo2posicao ? 0 : 180);
        servo2posicao = !servo2posicao;
        break;
      case 3:
        setColor(0,0,0);
        servo3.write(servo3posicao ? 0 : 180);
        servo3posicao = !servo3posicao;
        break;
    }
    contador = 0;
  }

  // Botão 2
  estado = digitalRead(botao2Pin);
  if (estado == LOW) {
    digitalWrite(relayPin, HIGH);
    delay(150000);
    digitalWrite(relayPin, LOW);
  }
}

// ------------------- Função para LEDs -------------------
void setColor(int red, int green, int blue) {
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue);
}

// ------------------- Dispensar Ipobrufeno -------------------
void dispensarIbuprofeno(int quantidade) {
  // Dispensar Medicamento
  for (int i = 0; i < quantidade; i++) {
    servo1.write(servo1posicao ? 0 : 180);
    servo1posicao = !servo1posicao;
    delay(1000);
  }

  // Dispensar Água
  digitalWrite(relayPin, HIGH);
  delay(150000);
  digitalWrite(relayPin, LOW);
}

// ------------------- Dispensar Paracetamol -------------------
void dispensarParacetamol(int quantidade) {
  // Dispensar Medicamento
  for (int i = 0; i < quantidade; i++) {
    servo2.write(servo2posicao ? 0 : 180);
    servo2posicao = !servo2posicao;
    delay(1000);
  }

  // Dispensar Água
  digitalWrite(relayPin, HIGH);
  delay(150000);
  digitalWrite(relayPin, LOW);
}

// ------------------- Dispensar Sedatif -------------------
void dispensarSedatif(int quantidade) {
  // Dispensar Medicamento
  for (int i = 0; i < quantidade; i++) {
    servo3.write(servo3posicao ? 0 : 180);
    servo3posicao = !servo3posicao;
    delay(1000);
  }

  // Dispensar Água
  digitalWrite(relayPin, HIGH);
  delay(150000);
  digitalWrite(relayPin, LOW);
}