#include <fauxmoESP.h>

#include <AsyncPrinter.h>
#include <async_config.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncTCPbuffer.h>
#include <SyncClient.h>
#include <tcp_axtls.h>



// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

#include <FastLED.h> //WS2812b
#include "DHT.h" //Temp sensor
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>        // Include the mDNS library

#include <ArduinoJson.h>

const char* wifi_ssid = "---";
const char* wifi_passwd = "---";
MDNSResponder mdns;

#define HTTP_REST_PORT 80 //default HTTP port
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50
#define LED_STRING_PIN     2
#define LED_PIN     5
#define NUM_LEDS    3
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define DHTPIN 4     //this is GPIO 2
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

fauxmoESP fauxmo; // enable alexa support, fake wemo

#define UPDATES_PER_SECOND 100

float humidity;
float temperature;
String location = "Desk";

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);


void GET_leds() {
  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& jsonObj = jsonBuffer.createObject();
    char JSONmessageBuffer[200];
        for (int i = 0; i < NUM_LEDS; i++) {
          jsonObj["led"+String(i)] = "["+String(leds[i].red)+","+String(leds[i].green)+","+String(leds[i].blue)+"]";
        }
              
        jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        Serial.println(JSONmessageBuffer);
        http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");   
        http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        
        http_rest_server.send(200, "application/json", JSONmessageBuffer);
}

void POST_leds() {
  http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");   //sets CORS headers, neccessary for POST request using "application/json" data types.
  http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  String message = "Number of args received:"; //creates plaintext representation of what the server received, sends as server response to confirm.
  message += http_rest_server.args();            //Get number of parameters
  for (int i = 0; i < http_rest_server.args(); i++) { 
    message += "Arg #" + (String)i + " –> ";   //Include the current iteration value
    message += http_rest_server.argName(i) + ": ";     //Get the name of the parameter
    message += http_rest_server.arg(i);              //Get the value of the parameter
  }
  Serial.println(message);
  http_rest_server.send(200, "text/plain", message);       //Response to the HTTP request 

  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
  StaticJsonBuffer<200> jsonBuffer; //creates a memory slot for incoming JSON, if data gets cut off or fails, increase this.
  JsonObject& JObject = jsonBuffer.parseObject(http_rest_server.arg("plain")); //creates object from parsed data held in .arg("plain"). 

  int red = JObject["red"];
  int green = JObject["green"];
  int blue = JObject["blue"];
  int id = JObject["id"];
  leds[id] = CRGB(red,green,blue);
  FastLED.show();
  
}

void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += http_rest_server.uri();
  message += "\nMethod: ";
  message += (http_rest_server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += http_rest_server.args();
  message += "\n";
  for (uint8_t i=0; i<http_rest_server.args(); i++){
    message += " " + http_rest_server.argName(i) + ": " + http_rest_server.arg(i) + "\n";
  }
  http_rest_server.send(404, "text/plain", message);
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
    http_rest_server.on("/leds", HTTP_OPTIONS, []() {
    http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");
    http_rest_server.sendHeader("Access-Control-Max-Age", "10000");
    http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    http_rest_server.send(200, "text/plain", "Pre-flight accepted" );
  });
    
    http_rest_server.on("/", HTTP_GET, []() {
    http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");   
    http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    http_rest_server.send(200, "text/html",
            "Welcome to the ESP8266 REST Web Server");
    });
    http_rest_server.on("/leds", HTTP_GET, GET_leds);
    http_rest_server.on("/leds", HTTP_POST, POST_leds);
    http_rest_server.on("/climate", HTTP_GET, GET_climate);
    http_rest_server.onNotFound(handleNotFound);
    

}

void GET_climate() {
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

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  pinMode(LED_STRING_PIN, OUTPUT);
  digitalWrite(LED_STRING_PIN, HIGH);
  
  fauxmo.addDevice("RGB Strip");
  
  if (init_wifi() == WL_CONNECTED) { // confirms connection
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
        if (mdns.begin("ws2812b", WiFi.localIP())) {
        Serial.println("MDNS responder started");
        }
        fauxmo.enable(true); //start fauxmo service
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
  }
  
  config_rest_server_routing(); // configures routing for REST responses

  http_rest_server.begin();
  Serial.println("HTTP REST Server Started");

   leds[0] = CRGB(255,0,0); 
   leds[1] = CRGB(0,255,0);
   leds[2] = CRGB(0,0,255);
   FastLED.show();


  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    Serial.print("Device "); Serial.print(device_name); 
    Serial.print(" state: ");
  if (state) {
    Serial.println("ON");
   leds[0] = CRGB(255,0,0); 
   leds[1] = CRGB(0,255,0);
   leds[2] = CRGB(0,0,255);
  } else {
    Serial.println("OFF");
   leds[0] = CRGB(255,0,0); 
   leds[1] = CRGB(0,255,0);
   leds[2] = CRGB(0,0,255);
  }
   FastLED.show();
  });
}

void loop() {
  http_rest_server.handleClient();
  fauxmo.handle();
  // Wait a few seconds between measurements.
}
