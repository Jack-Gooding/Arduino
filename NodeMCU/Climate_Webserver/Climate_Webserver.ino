

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const char* wifi_ssid = "--";
const char* wifi_passwd = "--";

#define HTTP_REST_PORT 80 //default HTTP port
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50
#define DHTPIN 4     // what digital pin we're connected to, this is PIN 2


#define DHTTYPE DHT22   // DHT 22  (AM2302)

float humidity;
float temperature;
String location = "Desk";

DHT dht(DHTPIN, DHTTYPE);



ESP8266WebServer http_rest_server(HTTP_REST_PORT);


void get_climate() {
    read_dht();
  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];
        jsonObj["temperature"] = temperature;
        jsonObj["humidity"] = humidity;
        jsonObj["location"] = location;        
        jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.println(JSONmessageBuffer);
        http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");   
        http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

int read_dht() { //Called to set up WiFi on boot
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // To read temperature as Fahrenheit (isFahrenheit = true)

  // Check if any reads failed and exit early (to try again).
  while (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
    temperature = dht.readTemperature();
  }
  Serial.print("Got data:");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");
}

int init_wifi() { //Called to set up WiFi on boot
    int retries = 0;

    Serial.println("Connecting to WiFi AP..........");

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_passwd);
    // check the status of WiFi connection to be WL_CONNECTED
    while ((WiFi.status() != WL_CONNECTED) && (retries < MAX_WIFI_INIT_RETRY)) {
        retries++;
        delay(WIFI_RETRY_DELAY);
        Serial.print("#");
    }
    return WiFi.status(); // return the WiFi connection status
}

void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_OPTIONS, []() {
    http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");
    http_rest_server.sendHeader("Access-Control-Max-Age", "10000");
    http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    http_rest_server.send(200, "text/plain", "" );
  });
    
    http_rest_server.on("/", HTTP_GET, []() {
    http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");   
    http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/climate", HTTP_GET, get_climate);

    

}

void setup() {
  Serial.begin(9600);
  read_dht(); //reads DHT sensor for temp/humidity
  if (init_wifi() == WL_CONNECTED) { // confirms connection
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
  }

  config_rest_server_routing(); // configures routing for REST responses

  http_rest_server.begin();
  Serial.println("HTTP REST Server Started");

  dht.begin();
}

void loop() {
  http_rest_server.handleClient();
  // Wait a few seconds between measurements.
}
