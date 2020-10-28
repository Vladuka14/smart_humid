#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define DHTPIN 4     // Номер пина, который подключен к DHT22
#define DHTTYPE DHT22   // Указываем, какой тип датчика мы используем

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 4
#define DHTTYPE DHT22     // DHT 22 (AM2302)

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

const char *mqtt_server = ""; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "usr"; // Логин от сервер
const char *mqtt_pass = "pass"; // Пароль от сервера
const char *ssid =  "wifi";  // Имя вайфай точки доступа
const char *pass =  "pass"; // Пароль от точки доступа

// Читаем MQTT сообщения
void callback(char* topic, byte* payload, unsigned int length) {}

WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);

void connect() {

  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);


  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Ошибка соединения WIFI. Проверьте данные: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(pass);
      Serial.println();
    }

    delay(500);
 
    if(millis() - wifiConnectStart > 5000) {
      Serial.println("Ошибка соединения WiFi");
      Serial.println("Попробуйте отправить обновленные параметры конфигурации.");
      return;
    }
  }

  Serial.println("WiFi соединение установлено");
  Serial.println("IP адрес: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);

  dht.begin();
  
  sensor_t sensor;

  delayMS = sensor.min_delay / 1000;
  while(!Serial) { }


  connect();

}

// Собираем данные, складываем воедино и отправляем
void report(double humidity, double tempC) {
  if (client.connect("ESP8266 Temperature and Humidity")) {
    if(client.publish("Humidifier/Metrics/Humidity", String(humidity)) == true){
      Serial.println(String(humidity));
      delay(2000);
    }
    if(client.publish("Humidifier/Metrics/Temp", String(tempC)) == true){
      delay(2000);
      ESP.deepSleep(300e6); // 20e6 это и есть deep-sleep. Засыпаем на 20 секунд!
    }
  }
}

void loop() {

bool toReconnect = false;
 Serial.println("tut");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Нет соединения WiFi");
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  float h = -999;
  float t = -999;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    t = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    h = event.relative_humidity;
  }

  if (h == -999 || t == -999) {
    Serial.println("Данных нет! Останавливаем цикл и запускаем по новой");
    return;
  }
  report(h, t);
}
