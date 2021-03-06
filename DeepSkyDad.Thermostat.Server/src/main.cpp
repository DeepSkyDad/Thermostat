#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP_EEPROM.h>
#include <NTPClient.h>

/***** CONSTANTS *****/

#define RELAY_CH1_BURNER 12 //D6
#define RELAY_CH2_HEATING_PUMP 14 //D5  
#define RELAY_CH3_WATER_PUMP 13 //D7

/***** VARIABLES *****/

struct SETTINGS
{
  int BURNER;
  int HEATING_PUMP;
  int WATER_PUMP;
  int CHECKSUM;
} _settings;

StaticJsonBuffer<400> _jsonBuffer;
ESP8266WebServer _server(80);

WiFiUDP ntpUDP;
int ntpUtcOffset = 1;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", ntpUtcOffset * 3600, 60000);
bool _timeSet = false;
bool _oddDay = false;
int _dayHours = -1;
int _dayMinutes;
int _day = 0;

bool _tempNodeFound = false;
String _tempNodeIp;
float _temperature = 100;
float _humidity = 0;

int refreshCurrentMillis = 0;
int refreshPrevMillis = 0;

/***** IMPLEMENTATION *****/

void eepromWrite()
{
  _settings.CHECKSUM = (_settings.BURNER + _settings.HEATING_PUMP + _settings.WATER_PUMP);
  EEPROM.put(0, _settings);
  EEPROM.commitReset();
}

void eepromInit()
{
  EEPROM.begin(sizeof(SETTINGS));
  EEPROM.get(0, _settings);
  bool isSettingsValid = (_settings.BURNER + _settings.HEATING_PUMP + _settings.WATER_PUMP) == _settings.CHECKSUM;

  if (!isSettingsValid)
  {
    _settings.BURNER = 0;
    _settings.HEATING_PUMP = 0;
    _settings.WATER_PUMP = 0;
    _settings.CHECKSUM = 0;
  }
}

void writeHeatingPumpLow()
{
  if (_temperature < 19)
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, HIGH);
  }
  else
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, LOW);
  }
}

void writeHeatingPumpHigh()
{
  if (_temperature < 22.5)
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, HIGH);
  }
  else
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, LOW);
  }
}

void writePins()
{
  if (_settings.BURNER == 2) //SUMMER
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, LOW);

    int dayHours = _dayHours + 2; //UTC+2 during summer...

    if (dayHours >= 19 && dayHours < 22)
    {
		digitalWrite(RELAY_CH1_BURNER, 1);
		if(dayHours == 19 && _dayMinutes >= 30 || dayHours > 19) {
			digitalWrite(RELAY_CH3_WATER_PUMP, 1);
		} else {
			digitalWrite(RELAY_CH3_WATER_PUMP, 0);
		}
    }
    else
    {
      digitalWrite(RELAY_CH1_BURNER, 0);
      digitalWrite(RELAY_CH3_WATER_PUMP, 0);
    }

    return;
  }

  digitalWrite(RELAY_CH1_BURNER, _settings.BURNER);

  if (_settings.HEATING_PUMP == 1)
  {
    writeHeatingPumpLow();
  }
  else if (_settings.HEATING_PUMP == 2)
  {
    writeHeatingPumpHigh();
  }
  else if (_settings.HEATING_PUMP == 3)
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, HIGH);
  }
  else if (_settings.HEATING_PUMP == 4)
  {
    if (_day == 0 || _day == 6)
    { //Sunday, Saturday
      if (_dayHours >= 7 && _dayHours < 20)
      {
        writeHeatingPumpHigh();
      }
      else
      {
        writeHeatingPumpLow();
      }
    }
    else
    { //Mon-Fri
      if ((_dayHours >= 7 && _dayHours < 8) || (_dayHours >= 16 && _dayHours < 20))
      {
        writeHeatingPumpHigh();
      }
      else
      {
        writeHeatingPumpLow();
      }
    }
  }
  else if (_settings.HEATING_PUMP == 5)
  {
    //Holiday
    if (_dayHours >= 7 && _dayHours < 20)
    {
      writeHeatingPumpHigh();
    }
    else
    {
      writeHeatingPumpLow();
    }
  }
  else
  {
    digitalWrite(RELAY_CH2_HEATING_PUMP, LOW);
  }

  if (_settings.WATER_PUMP == 0 || _settings.WATER_PUMP == 1)
  {
    digitalWrite(RELAY_CH3_WATER_PUMP, _settings.WATER_PUMP);
  }
  else if (_settings.WATER_PUMP == 2)
  {
    //water heat auto mode
    if (_dayHours >= 19 && _dayHours < 23)
    {
      digitalWrite(RELAY_CH3_WATER_PUMP, HIGH);
    }
    else
    {
      digitalWrite(RELAY_CH3_WATER_PUMP, LOW);
    }
  }
}

void wwwSendData()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["burner"] = _settings.BURNER;
  data["heating_pump"] = _settings.HEATING_PUMP;
  data["water_pump"] = _settings.WATER_PUMP;
  data["temperature"] = _temperature;
  data["humidity"] = _humidity;
  data["relay_burner"] = digitalRead(RELAY_CH1_BURNER);
  data["relay_heating_pump"] = digitalRead(RELAY_CH2_HEATING_PUMP);
  data["relay_water_pump"] = digitalRead(RELAY_CH3_WATER_PUMP);
  data["odd_day"] = _oddDay;
  data["day_hours"] = _dayHours;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void wwwGetTime()
{
  JsonObject &data = _jsonBuffer.createObject();
  data["formattedTime"] = timeClient.getFormattedTime();;
  String json;
  data.printTo(json);
  _jsonBuffer.clear();
  _server.send(200, "text/json", json);
}

void wwwSaveData()
{
  JsonObject &root = _jsonBuffer.parseObject(_server.arg("plain"));

  if (root.containsKey("burner"))
    _settings.BURNER = root.get<int>("burner");
  if (root.containsKey("heating_pump"))
    _settings.HEATING_PUMP = root.get<int>("heating_pump");
  if (root.containsKey("water_pump"))
    _settings.WATER_PUMP = root.get<int>("water_pump");

  writePins();
  _jsonBuffer.clear();
  eepromWrite();
  wwwSendData();
}

void updateTime()
{
  if (!_timeSet)
  {
    _timeSet = timeClient.update();
  }

  if (_timeSet)
  {
    _oddDay = (timeClient.getEpochTime() / 86400L) % 2 != 0;
    _dayHours = timeClient.getHours();
    _dayMinutes = timeClient.getMinutes();
    _day = timeClient.getDay();
  }
}

void updateSensorData()
{
  if (!_tempNodeFound)
  {
    int n = MDNS.queryService("esp", "tcp");

    if (n > 0)
    {
      _tempNodeFound = true;
      _tempNodeIp = String(MDNS.IP(0)[0]) + String(".") +
                    String(MDNS.IP(0)[1]) + String(".") +
                    String(MDNS.IP(0)[2]) + String(".") +
                    String(MDNS.IP(0)[3]);
    }
  }

  if (_tempNodeFound)
  {
    HTTPClient http;
    http.begin("http://" + _tempNodeIp + "/api/status");
    int httpCode = http.GET();
    if (httpCode == 200)
    {
      String response = http.getString();
      JsonObject &root = _jsonBuffer.parseObject(response);
      _temperature = root.get<float>("temperature");
      _humidity = root.get<float>("humidity");
      _jsonBuffer.clear();
    }
    http.end(); //Close connection
  }
}

void setup()
{
  //initialize pins
  pinMode(RELAY_CH1_BURNER, OUTPUT);
  pinMode(RELAY_CH2_HEATING_PUMP, OUTPUT);
  pinMode(RELAY_CH3_WATER_PUMP, OUTPUT);
  digitalWrite(RELAY_CH1_BURNER, LOW);
  digitalWrite(RELAY_CH3_WATER_PUMP, LOW);
  digitalWrite(RELAY_CH3_WATER_PUMP, LOW);

  //read eeprom settings
  eepromInit();

  //start searial
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //connect to wifi
  WiFi.begin("v&p", "jovanovic64");

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

  //enable over the air update, e.g. in PlatformIO run this commands:
  //for firmware: "platformio run --target upload --upload-port 10.20.1.140"
  //for file system image: "platformio run --target uploadfs --upload-port 10.20.1.140"
  ArduinoOTA.setHostname("termostat");
  ArduinoOTA.begin();

  MDNS.begin("termostat");

  //start NTP client, read time
  timeClient.begin();

  //start web server
  SPIFFS.begin();

  updateTime();
  updateSensorData();
  writePins();

  _server.on("/api/status", HTTP_GET, []() {
    wwwSendData();
  });
   _server.on("/api/time", HTTP_GET, []() {
    wwwGetTime();
  });
  _server.on("/api/save", HTTP_POST, []() {
    wwwSaveData();
  });

  _server.serveStatic("/", SPIFFS, "/www/");

  _server.begin();
}

void loop()
{
  refreshCurrentMillis = millis();
  if (refreshCurrentMillis - refreshPrevMillis > 30000)
  {
    refreshPrevMillis = refreshCurrentMillis;

    updateTime();
    updateSensorData();
    writePins();
  }

  ArduinoOTA.handle();

  _server.handleClient();
}
