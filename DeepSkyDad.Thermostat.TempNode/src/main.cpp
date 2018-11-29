#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>

/***** CONSTANTS *****/

#define ONE_WIRE_PIN 14           //D5

StaticJsonBuffer<400> _jsonBuffer;
ESP8266WebServer _server(80);

/***** IMPLEMENTATION *****/

void wwwSendData()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["temperature"] = 123;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void setup()
{
  //initialize pins
  pinMode(ONE_WIRE_PIN, INPUT);

  //start searial
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //connect to wifi
  WiFi.begin("v&p", "jovanovic64");

  MDNS.begin("termostat");

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
}