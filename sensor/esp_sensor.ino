/*
 * ESP-Sensor
 */
// INCLUDES
//***************************************************************
#include <ESP8266WiFi.h>
#include "SdsDustSensor.h"
#include "DHT.h"
#include <ArduinoJson.h>
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// DEFINES
//***************************************************************
//SDS011
#define SDSRX D1
#define SDSTX D2

//DHT
#define DHTPIN D5
#define DHTTYPE DHT22

// WIFI
#define WIFI_SSID "************" //your wifi ssid and password
#define WIFI_PASS "************"

//MQTT
#define MQTT_SERVER "******" //mqtt host
#define MQTT_PORT 1883
#define MQTT_USER "******" //Admin User
#define MQTT_PASS "******" //Admin Pass
#define MQTT_TOPIC "******" //Topic you want to write data to

//MISC
#define ESPID "************" //just a random ID or name to indentify your sensor
#define LON 0.000 //longitude
#define LAT 00.000 //latitude
#define ENV "************" //Environment: Examples: "room", "garden", "street", "balcony"...

const long interval = 60000; //intervall you want to measure (in ms)
const int airqualityInterval = 3; //Interval you want to measure the air quality (in minutes)

unsigned long previousMillis = 0;

// DECLARES
//***************************************************************
DHT dht(DHTPIN, DHTTYPE);
SdsDustSensor sds(SDSRX, SDSTX);

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

//***************************************************************

void setup() {
  Serial.begin(9600);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setMaxTopicLength(256);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCredentials(MQTT_USER, MQTT_PASS);

  dht.begin();

  sds.begin();
  Serial.println(sds.setActiveReportingMode().toString());// ensures sensor is in 'active' reporting mode
  Serial.println(sds.setCustomWorkingPeriod(airqualityInterval).toString()); // sends data every 3 minutes

  connectToWifi();
}

void loop() {
  unsigned long currentMillis = millis();

  //Runs every 60 seconds (interval)
  if (currentMillis - previousMillis >= interval){
    previousMillis = currentMillis;
    Serial.println();

    unsigned long epoch_time = Get_Epoch_Time(); //Gets current UNIX
    Serial.print("Epoch Time: ");
    Serial.println(epoch_time);

    StaticJsonDocument<256> doc; //create json
    doc["id"] = ESPID;
    doc["lon"] = LON;
    doc["lat"] = LAT;
    doc["env"] = ENV;
    doc["time"] = epoch_time;

    //reads sds values
    PmResult pm = sds.readPm();
    if(pm.isOk()){
      Serial.println(pm.toString());
    } else{
      Serial.print("Could not read values from sensor, reason: ");
      Serial.println(pm.statusToString());
    }
    doc["sds011"]["pm10"] = pm.pm10;
    doc["sds011"]["pm25"] = pm.pm25;

    //reads temperature and humidity
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    doc["dht"]["temp"] = temp;
    doc["dht"]["hum"] = hum;

    Serial.print("Temperatur: ");
    Serial.print(temp);
    Serial.println("°C");
    Serial.print("Luftfeuchtigkeit: ");
    Serial.print(hum);
    Serial.println("%");

    //create jsonbuffer and send it to mqtt
    char jsonbuffer[256];
    serializeJson(doc, jsonbuffer);

    uint16_t packetId = mqttClient.publish(MQTT_TOPIC, 2, false, jsonbuffer);
    Serial.print("Publishing at QoS 2, packetId: ");
    Serial.println(packetId);
  }
}

//connect to wifi
void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

//connect to mqtt-server
void connectToMqtt(){
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

//runs if esp is connected to wifi
void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

//runs if esp is disconnected from wifi
void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

//runs if esp is connected to mqtt
void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

//runs if esp is disconnected from esp
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

//runs if esp published on mqtt
void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

//gets current UNIX time
unsigned long Get_Epoch_Time() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}
