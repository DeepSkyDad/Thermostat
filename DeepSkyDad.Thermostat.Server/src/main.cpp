#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP_EEPROM.h>
#include <NTPClient.h>

/***** CONSTANTS *****/

#define RELAY_CH1_BURNER 14           //D5
#define RELAY_CH2_HEATING_PUMP 12     //D6
#define RELAY_CH3_WATER_PUMP 13       //D7

/***** VARIABLES *****/

struct SETTINGS {
  int BURNER;
  int HEATING_PUMP;
  int WATER_PUMP;
  int CHECKSUM;
} _settings;

StaticJsonBuffer<400> _jsonBuffer;
ESP8266WebServer _server(80);

WiFiUDP ntpUDP;
int ntpUtcOffset = 1;
int ntpCurrentMillis = 0;
int ntpPreviousMillis = 0;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", ntpUtcOffset*3600, 60000);

/***** IMPLEMENTATION *****/

void eepromWrite() {
  EEPROM.put(0, _settings);
  EEPROM.commitReset();
}

void eepromInit() {
  EEPROM.begin(sizeof(SETTINGS));
  EEPROM.get(0, _settings);
  bool isSettingsValid = _settings.CHECKSUM >=0 &&
    _settings.CHECKSUM <= 3 &&
    (_settings.BURNER + _settings.HEATING_PUMP + _settings.WATER_PUMP) == _settings.CHECKSUM;

  if(!isSettingsValid) {
    _settings.BURNER = 0;
    _settings.HEATING_PUMP = 0;
    _settings.WATER_PUMP = 0;
    _settings.CHECKSUM = 0;
    eepromWrite();
  }
}

void readPins() {
  _settings.BURNER = digitalRead(RELAY_CH1_BURNER);
  _settings.HEATING_PUMP = digitalRead(RELAY_CH2_HEATING_PUMP);
  _settings.WATER_PUMP = digitalRead(RELAY_CH3_WATER_PUMP);
}

void writePins() {
  digitalWrite(RELAY_CH1_BURNER, _settings.BURNER);
  digitalWrite(RELAY_CH2_HEATING_PUMP, _settings.HEATING_PUMP);
  digitalWrite(RELAY_CH3_WATER_PUMP, _settings.WATER_PUMP);
}

void wwwSendData()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["burner"] = _settings.BURNER;
  data["heating_pump"] = _settings.HEATING_PUMP;
  data["water_pump"] = _settings.WATER_PUMP;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void wwwSaveData()
{
  JsonObject &root = _jsonBuffer.parseObject(_server.arg("plain"));
  _settings.BURNER = root.get<int>("burner");
  _settings.HEATING_PUMP = root.get<int>("heating_pump");
  _settings.WATER_PUMP = root.get<int>("water_pump");
  writePins();
  _jsonBuffer.clear();

  wwwSendData();
}

void setup()
{
  //initialize pins
  pinMode(RELAY_CH1_BURNER, OUTPUT);
  pinMode(RELAY_CH2_HEATING_PUMP, OUTPUT);
  pinMode(RELAY_CH3_WATER_PUMP, OUTPUT);

  //read eeprom and write relay pins
  eepromInit();
  writePins();

  //start searial
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //connect to wifi
  WiFi.begin("xxxx", "xxxxx");

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

  //start NTP client
  timeClient.begin();
  timeClient.update();

  //start web server
  SPIFFS.begin();

  _server.on("/api/status", HTTP_GET, []() {
    wwwSendData();
  });
  _server.on("/api/wwwSaveData", HTTP_POST, []() {
    wwwSaveData();
  });

  _server.serveStatic("/", SPIFFS, "/www/");

  _server.begin();
}

void loop()
{
  _server.handleClient();

  ntpCurrentMillis = millis();
  if (ntpCurrentMillis - ntpPreviousMillis > 1000) {
    ntpPreviousMillis = ntpCurrentMillis; 
    timeClient.update();
    //printf("Time Epoch: %d: ", timeClient.getEpochTime());
    //Serial.println(timeClient.getFormattedTime());
  }
}
