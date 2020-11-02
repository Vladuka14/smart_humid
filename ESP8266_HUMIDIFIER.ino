#include "functions.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

//маппинг портов ESP8266
int D0   = 16;
int D1   = 5;
int D2   = 4;
int D3   = 0;
int D4   = 2;
int D5   = 14;
int D6   = 12;
int D7   = 13;
int D8   = 15;
int D9   = 3;
int D10  = 1;

//входы
#define ONE_WIRE_BUS 2
int fun = D1;
int kRecvPin = D5;
int led1 = D2;
int led2 = D7;
int led3 = D6;
int btn = D0;
int sonar = D8;


//Переменные
unsigned long timing;
int sonar_mode = 0;
bool sonarPower = 0;
bool powerON = 0;
int sonartime = 0;
bool firstStart = 1;
int dif;
int autoMod;
int max_hum = 50;
bool autowork = 0;
int autoworkMode = 0;
bool btnPress = 0;

#define BUFFER_SIZE 100

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Настройка WIFI и MQTT
const char *ssid =  "Home_180";  // Имя вайфай точки доступа
const char *pass =  "dacha2017"; // Пароль от точки доступа

const char *mqtt_server = "192.168.31.25"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "vlad"; // Логин от сервер
const char *mqtt_pass = "qpalzm123"; // Пароль от сервера

WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);

//Ик приемник
IRrecv irrecv(kRecvPin);
decode_results results;


void setup() {
  sensors.begin();
  Serial.begin(115200);

  irrecv.enableIRIn(); //Включение ИК
  
  pinMode(fun, OUTPUT);
  digitalWrite(fun, LOW);
  pinMode(sonar, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(btn, INPUT);
}


void loop() {
  /*
   * Режимы работы увлажнителя
   */
  if (sonar_mode > 0) {
    if (powerON == 1) {
      sonartime = 10000 * sonar_mode;
    }
    else if (firstStart == 1) {
      timing = 0;
      sonartime = 0;
    }
    else if (powerON == 0){
      sonartime = 10000;
    }
    if (sonar_mode == 3 && powerON == 0) {
      digitalWrite(sonar, HIGH);
      Serial.println ("Включили 3"); //Отладочная информация
      powerON = 1;
    } else if (sonar_mode != 3 && millis() - timing > sonartime && firstStart == 0) {
      timing = millis();
      digitalWrite(sonar, !sonarPower);
      sonarPower = !sonarPower;
      powerON = !powerON;
      firstStart = 0;
    } else if (firstStart == 1) {
      timing = millis();
      digitalWrite(sonar, 1);
      sonarPower = 1;
      powerON = 1;
      firstStart = 0;
    }
  } else {
    digitalWrite(sonar, LOW);
  }

  /*
   * Установка режима с пульта управления
   */
  if (irrecv.decode(&results)) {
    if (results.value == 0x80C || results.value == 0xC || results.value == 0x569579DF) {
      autowork = 0;
      startHum(0, 1);
    }
    if ((results.value == 0x801 || results.value == 0x1 || results.value == 0xFF30CF) && sonar_mode != 1) {
      startHum(1, 3);
    }
    if ((results.value == 0x802 || results.value == 0x2 || results.value == 0xFF9867) && sonar_mode != 2) {
      startHum(2, 3);
    }
    if ((results.value == 0x803 || results.value == 0x3 || results.value == 0xFF7A85) && sonar_mode != 3) {
      startHum(3, 3);
    }
    irrecv.resume();  // Receive the next value
  }


  /*
   * Подключение по WiFi
   */
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
    Serial.println("WiFi connected");
  }

  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect("arduinoClient2")
                         .set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
        client.subscribe("Humidifier/workmode"); // подписывааемся по топик с данными для светодиода
        client.subscribe("Humidifier/workmode/speed");
      } else {
        Serial.println("Could not connect to MQTT server");   
      }
    }
    if (client.connected()){
      client.loop();
    }
  }
}