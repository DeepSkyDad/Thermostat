#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include "DHTesp.h"

/***** CONSTANTS *****/
  
#define DHT22_PIN 4

/***** VARIABLES *****/

StaticJsonBuffer<400> _jsonBuffer;
ESP8266WebServer _server(80);

DHTesp _dht;
float _temperatureCelsius = -127;
float _humidity = 0;
unsigned long _tempRefreshLastMillis;
unsigned long _tempRefreshPeriodMilis = 5000;

/***** IMPLEMENTATION *****/

void wwwSendData()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["temperature"] = _temperatureCelsius;
  data["humidity"] = _humidity;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void readTempSensor()
{
  TempAndHumidity newValues = _dht.getTempAndHumidity();
  _temperatureCelsius = newValues.temperature;
  _humidity = newValues.humidity;
}

void setup()
{
  //initialize pins
  pinMode(DHT22_PIN, INPUT);

  //start searial
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //begin temperature sensor
  _dht.setup(DHT22_PIN, DHTesp::DHT22);
  readTempSensor();

  //connect to wifi
  WiFi.begin("xxx", "xxx");

  Serial.println();
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("success!");
  Serial.print("IP Address is: ");
  Serial.println(WiFi.localIP());

  //enable over the air update, e.g. in PlatformIO run this command: "platformio run --target upload --upload-port 10.20.1.138"
  ArduinoOTA.setHostname("temperature");
  ArduinoOTA.begin();

  MDNS.begin("temperature");
  MDNS.addService("esp", "tcp", 8080);

  _server.on("/api/status", HTTP_GET, []() {
    wwwSendData();
  });

  _server.begin();
}

void loop()
{
  ArduinoOTA.handle();

  _server.handleClient();

  if (millis() > _tempRefreshLastMillis + _tempRefreshPeriodMilis)
  {
    readTempSensor();
    _tempRefreshLastMillis = millis();
    Serial.print("Temperature: ");
    Serial.println(_temperatureCelsius);
    Serial.print("Humidity: ");
    Serial.println(_humidity);
    Serial.println(" C");
  }
}
