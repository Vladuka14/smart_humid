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

//Переменные
int dif;
int autoMod;

unsigned long timing;
int sonar_mode = 0;
bool sonarPower = 0;
bool powerON = 0;
int sonartime = 0;
bool firstStart = 1;


int sonar = D8; //ok+



int fun = D1; //ok
int kRecvPin = D5; //ok

int led1 = D2; //ok+
int led2 = D7; //ok+
int led3 = D6; //ok+

int btn = D0;

int max_hum = 50;

bool autowork = 0;
int autoworkMode = 0;

//MQTT
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

//ИК приемник
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const char *ssid =  "WIFI_NAME";  // Имя вайфай точки доступа
const char *pass =  "Wifi_PASS"; // Пароль от точки доступа

const char *mqtt_server = "MQTT_IP"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "user"; // Логин от сервер
const char *mqtt_pass = "Pass"; // Пароль от сервера

#define BUFFER_SIZE 100

void callback(const MQTT::Publish& pub)
{  
  String payload = pub.payload_string(); 
  String topic = pub.topic();
  if(topic == "Humidifier/workmode")
  {
    Serial.println("Управляем c MQTT");
    int stled = payload.toInt(); // преобразуем полученные данные в тип integer
    startHum(stled, 1);
  }
  if(topic == "Humidifier/workmode/speed" ) // проверяем из нужного ли нам топика пришли данные 
  {
    Serial.print("Управляем c MQTT");
    int stled = payload.toInt(); // преобразуем полученные данные в тип integer
    startHum(stled, 2);
  }
}

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


bool btnPress = 0;
void loop() {
//  if (digitalRead(btn) == HIGH && btnPress == 0){ //если кнопка нажата
//    btnPress = 1;
//  } else if (digitalRead(btn) == LOW && (btnPress == 1 || sonar_mode)){
//    startHum(0, -1);
//    btnPress = 0;
//  }
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
      Serial.println ("Включили 3");
      powerON = 1;
    } else if (sonar_mode != 3 && millis() - timing > sonartime && firstStart == 0) { // Вместо 10000 подставьте нужное вам значение паузы
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

//принимаем ик
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
} // конец основного цикла


void startHum(int workMode, int mqtt) {
  Serial.print(workMode);
  if (mqtt == 1){
    client.publish("Humidifier/status/",String(workMode));
  } else if(mqtt == 2){
     client.publish("Humidifier/status/speed2",String(workMode));
  } else if (mqtt == 3){
     client.publish("Humidifier/status/",String(1));
     client.publish("Humidifier/status/speed2",String(workMode));
  } else if (mqtt == -1) {
    client.publish("Humidifier/status/",String(0));
    client.publish("Humidifier/status/speed2",String(0));
    client.publish("Humidifier/errorStatus/",String("Верх поднят"));
  }
 
  dropLedMode();
  if (workMode >= 1) {
    digitalWrite(led1, HIGH);
    analogWrite(fun, 10);
  }
  if (workMode >= 2) {
    digitalWrite(led2, HIGH);
    analogWrite(fun, 20);
  }
  if (workMode >= 3) {
    digitalWrite(led3, HIGH);
    analogWrite(fun, 255);
  }
  if (workMode == 0) {
    analogWrite(fun, 0);
  }

  firstStart = 1;
  timing = 0;
  sonar_mode = workMode;
  powerON = 0;
}
void dropLedMode() {
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
}
