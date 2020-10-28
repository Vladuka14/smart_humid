void callback(const MQTT::Publish& pub)
{  
  String payload = pub.payload_string(); 
  String topic = pub.topic();
  if(topic == "Humidifier/workmode")
  {
    int stled = payload.toInt(); // преобразуем полученные данные в тип integer
    startHum(stled, 1);
  }
  if(topic == "Humidifier/workmode/speed" ) // проверяем из нужного ли нам топика пришли данные 
  {
    int stled = payload.toInt(); // преобразуем полученные данные в тип integer
    startHum(stled, 2);
  }
}
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
