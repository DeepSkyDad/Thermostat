#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>

/***** CONSTANTS *****/

#define DS18B20_PIN 4

/***** VARIABLES *****/

StaticJsonBuffer<400> _jsonBuffer;
ESP8266WebServer _server(80);

OneWire _ds(DS18B20_PIN);
DallasTemperature _sensors(&_ds);
float _temperatureCelsius = -127;
unsigned long _tempRefreshLastMillis;
unsigned long _tempRefreshPeriodMilis = 5000;

/***** IMPLEMENTATION *****/

void wwwSendData()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["temperature"] = _temperatureCelsius;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void readTempSensor()
{
	_sensors.requestTemperatures();
	_temperatureCelsius = _sensors.getTempCByIndex(0);
}

void setup()
{
  //initialize pins
  pinMode(DS18B20_PIN, INPUT);

  //start searial
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //begin temperature sensor
  _sensors.begin();
  readTempSensor();

  //connect to wifi
  WiFi.begin("xxx", "xxx");

  MDNS.begin("temperature");

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

  _server.on("/api/status", HTTP_GET, []() {
    wwwSendData();
  });

  _server.begin();
}

void loop()
{
  _server.handleClient();

  if(millis() > _tempRefreshLastMillis + _tempRefreshPeriodMilis) {
		readTempSensor();
		_tempRefreshLastMillis = millis();
     Serial.print("Temperature refreshed: ");
     Serial.print(_temperatureCelsius);
     Serial.println(" C");
	}
}
