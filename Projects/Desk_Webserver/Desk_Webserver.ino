//https://github.com/kakopappa/arduino-esp8266-alexa-wemo-switch/blob/master/sinric.ino

#include <FastLED.h> //WS2812b
#include "DHT.h" //Temp sensor
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <PubSubClient.h> //MQTT
#include <ESP8266mDNS.h> //mDNS, for local DNS service "device.local"
#include <ArduinoJson.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h> //OLED graphics processor
#include <Adafruit_SSD1306.h> //OLED controller

#define LED_PIN     5
#define NUM_LEDS    3
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define SCREEN_LED_PIN 0
String screenLightsState = "off";

#define DHTPIN 12     //this is GPIO 2
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);

#define UPDATES_PER_SECOND 100

#define OLED_RESET LED_BUILTIN  //4
Adafruit_SSD1306 display(OLED_RESET);

unsigned long previousMillis = 0;        // will store last time temp/humid was updated
const long interval = 120000; //(1000*60*2) //2 min

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

float humidity;
float temperature;
String location = "Desk";

#define MQTT_MAX_PACKET_SIZE 256;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// prototypes
boolean connectWifi();

// Change this!!
const char* ssid = "---";
const char* password = "---";
const char* deviceName = "Desk Webserver";

const char* mqtt_server = "jack-gooding.com";

boolean wifiConnected = false;


ESP8266WebServer server(80);
WiFiClient wifiClient;

void callback(char* topic, byte* message, unsigned int length) {
  String payload_buff;
  for (int i=0;i<length;i++) {
    payload_buff = payload_buff+String((char)message[i]);
  }
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(payload_buff); // Print out messages

  if (String(topic) == "desk/lights") {
    Serial.println(payload_buff);
    Serial.println(payload_buff.toInt());
    if (payload_buff.toInt() > 0) {
      analogWrite(SCREEN_LED_PIN, 0);
    } else {
      analogWrite(SCREEN_LED_PIN, 1023);
    }
  }

}

PubSubClient client(mqtt_server, 1883, callback, wifiClient);

void GET_leds() {
  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
    StaticJsonDocument<400> jsonObj;
        for (int i = 0; i < NUM_LEDS; i++) {
          jsonObj["led"+String(i)] = "["+String(leds[i].red)+","+String(leds[i].green)+","+String(leds[i].blue)+"]";
        }

        //jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        //Serial.println(JSONmessageBuffer);
        //server.sendHeader("Access-Control-Allow-Origin", "*");
        //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        char buffer[400];
        serializeJsonPretty(jsonObj, buffer);
        server.send(200, "application/json", buffer);
        //server.send(200, "application/json", Serial);
}

void POST_leds() {
  //server.sendHeader("Access-Control-Allow-Origin", "*");   //sets CORS headers, neccessary for POST request using "application/json" data types.
  //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  String message = "Number of args received:"; //creates plaintext representation of what the server received, sends as server response to confirm.
  message += server.args();            //Get number of parameters
  for (int i = 0; i < server.args(); i++) {
    message += "Arg #" + (String)i + " –> ";   //Include the current iteration value
    message += server.argName(i) + ": ";     //Get the name of the parameter
    message += server.arg(i);              //Get the value of the parameter
  }
  Serial.println(message);
  server.send(200, "text/plain", message);       //Response to the HTTP request

  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
  //StaticSerial<200> Serial; //creates a memory slot for incoming JSON, if data gets cut off or fails, increase this.
  //JsonObject& JObject = Serial.parseObject(server.arg("plain")); //creates object from parsed data held in .arg("plain").

  StaticJsonDocument<400> JObject;
  DeserializationError error = deserializeJson(JObject, server.arg("plain"));

  int red = JObject["red"];
  int green = JObject["green"];
  int blue = JObject["blue"];
  int id = JObject["id"];
  leds[id] = CRGB(red,green,blue);
  FastLED.show();

}

void GET_screen() {
  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
        StaticJsonDocument<400> jsonObj;
        jsonObj["onState"] = screenLightsState;

        //jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        //Serial.println(JSONmessageBuffer);
        //server.sendHeader("Access-Control-Allow-Origin", "*");
        //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        char buffer[400];
        serializeJsonPretty(jsonObj, buffer);
        server.send(200, "application/json", buffer);
}

void POST_screen() {
  //server.sendHeader("Access-Control-Allow-Origin", "*");   //sets CORS headers, neccessary for POST request using "application/json" data types.
  //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
  //StaticSerial<200> Serial; //creates a memory slot for incoming JSON, if data gets cut off or fails, increase this.
  //JsonObject& JObject = Serial.parseObject(server.arg("plain")); //creates object from parsed data held in .arg("plain").
  StaticJsonDocument<400> JObject;
  DeserializationError error = deserializeJson(JObject, server.arg("plain"));
  String onState = JObject["onState"];

  if (onState == "on") {
    analogWrite(SCREEN_LED_PIN, 0);
    screenLightsState = "on";
  }
   else {
    analogWrite(SCREEN_LED_PIN, 1023);
    screenLightsState = "off";
  }

  //char JSONmessageBuffer[200];
  JObject["onState"] = screenLightsState;
  char buffer[400];
  serializeJsonPretty(JObject, buffer);
  server.send(200, "application/json", buffer);
  //JObject.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  //Serial.println(JSONmessageBuffer);
  //server.sendHeader("Access-Control-Allow-Origin", "*");
  //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
  //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

  //server.send(200, "application/json", Serial);
}

void GET_climate() {

  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
        StaticJsonDocument<400> jsonObj;
        jsonObj["temperature"] = temperature;
        jsonObj["humidity"] = humidity;
        jsonObj["location"] = location;
        //jsonObj.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
        //Serial.println(JSONmessageBuffer);
        char buffer[400];
        serializeJsonPretty(jsonObj, buffer);
        server.send(200, "application/json", buffer);
        //server.sendHeader("Access-Control-Allow-Origin", "*");
        //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
        //server.send(200, "application/json", server);
}

int read_dht() { //Called to set up WiFi on boot
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();

  // To read temperature as Fahrenheit (isFahrenheit = true)
  int i = 0;
  // Check if any reads failed and exit early (to try again).
  while (isnan(humidity) || isnan(temperature)) {
    i++;
    resetDisplay();
    display.println("DHT Err!");
    display.print("x");
    display.println(i);
    display.display();
    Serial.print("Failed to read from DHT sensor! x");
    Serial.println(i);
    delay(2000);
    humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
    temperature = dht.readTemperature();
    if (i >= 10) {
      humidity = 0;
      temperature = 0;
    }
  }
  Serial.print("Got data:");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");

  setDisplay();
}

void setDisplay() {
  resetDisplay();

  display.print("T: ");
  display.print(temperature,1);
  display.println("C");
  display.setTextSize(2);
  display.print("H: ");
  display.print(humidity,1);
  display.println("%");
  display.display();
}

void resetDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
}


boolean connectMQTT(){
  if (client.connected()){
    return true;
  }

  Serial.print("Connecting to MQTT server ");
  Serial.print(mqtt_server);
  Serial.print(" as ");
  //Serial.println(host);

  if (client.connect("Desk Webserver")) {
    Serial.println("Connected to MQTT broker");
    return true;
  }
  else {
    Serial.println("MQTT connect failed! ");
    return false;
  }
}

void disconnectMQTT(){
  client.disconnect();
}

void mqtt_handler(){

  client.loop();
  delay(100); //let things happen in background
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  pinMode(SCREEN_LED_PIN, OUTPUT);
  analogWrite(SCREEN_LED_PIN, 1023);
  // Initialise wifi connection

  Serial.println("Device turned on");

  wifiConnected = connectWifi();

  if(wifiConnected){
    server.on("/", HTTP_GET, [](){
    server.send(200, "text/plain", "This is an example index page your server may send.");
    });

    server.on("/leds", HTTP_OPTIONS, []() {
      //server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Max-Age", "10000");
      server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      server.send(200, "text/plain", "Pre-flight accepted" );
    });
    server.on("/screen-lights", HTTP_OPTIONS, []() {
      //server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Max-Age", "10000");
      server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      server.send(200, "text/plain", "Pre-flight accepted" );
    });
    server.on("/", HTTP_GET, []() {
      //server.sendHeader("Access-Control-Allow-Origin", "*");
      //server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
      //server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
      server.send(200, "text/html", "Welcome to the ESP8266 REST Web Server");
    });
    server.on("/leds", HTTP_GET, GET_leds);
    server.on("/leds", HTTP_POST, POST_leds);
    server.on("/screen-lights", HTTP_GET, GET_screen);
    server.on("/screen-lights", HTTP_POST, POST_screen);
    server.on("/climate", HTTP_GET, GET_climate);

    server.onNotFound([](){
        server.send(404, "text/plain", "Not found");
    });

    server.begin();
  } else {
    delay(500);
    while (1)
    {

      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      resetDisplay();
      display.println("WiFi err:");
      display.println("No Signal!");
      display.display();
      delay(2500);
    }
  }

   leds[0] = CRGB(255,0,0);
   leds[1] = CRGB(0,255,0);
   leds[2] = CRGB(0,0,255);
   FastLED.show();
   delay(200);
}


void loop() {
  delay(1);
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    read_dht();
    // if the LED is off turn it on and vice-versa:

    // set the LED with the ledState of the variable:
  }
}


//***Pubsub Reconnect Function***
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Desk Server")) {
      Serial.println("connected");
      client.subscribe("device/on");
      client.subscribe("device/state");
      client.subscribe("desk/lights");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void Desk_OLED_Changed(uint8_t brightness) {
    Serial.print("Device 1 changed to ");

    //do what you need to do here

    //EXAMPLE
    if (brightness > 0) {
      Serial.println("ON");
      setDisplay();
    }
    else if (brightness == 0) {
      Serial.println("OFF");
      display.clearDisplay();
      display.display();
    }
    else {
      Serial.print("DIM "); Serial.println(brightness);
    }
}

void Desk_RGBs_Changed(uint8_t brightness) {
    Serial.print("Device 2 changed to ");

    //do what you need to do here

    //EXAMPLE
    if (brightness == 255) {
      Serial.println("ON");
      leds[0] = CRGB(255,0,0);
      leds[1] = CRGB(0,255,0);
      leds[2] = CRGB(0,0,255);
    }
    else if (brightness == 0) {
      Serial.println("OFF ");
      leds[0] = CRGB(0,0,0);
      leds[1] = CRGB(0,0,0);
      leds[2] = CRGB(0,0,0);

    }
    else {
      Serial.print("DIM "); Serial.println(brightness);
      leds[0] = CRGB(brightness,0,0);
      leds[1] = CRGB(0,brightness,0);
      leds[2] = CRGB(0,0,brightness);
    }
    FastLED.show();
}

void Screen_Lights_Changed(uint8_t brightness) {
    if (brightness == 255) {
    analogWrite(SCREEN_LED_PIN, 0);
    } else {
    analogWrite(SCREEN_LED_PIN, 1023);
    }
}

boolean connectWifi(){
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.hostname("Desk Server");
  WiFi.begin(ssid, password);

  // Wait for connection
  Serial.println("");
  Serial.print("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if ( i % 16 == 0) {
      resetDisplay();
      display.print("WiFi:");
      display.display();
    } else {
      display.print(".");
      display.display();
    }
    Serial.print(".");
    if (i > 50){
      state = false;
      break;
    }
    i++;
  }

  if (!MDNS.begin("Desk Server")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    resetDisplay();
    display.print(WiFi.localIP());
    display.display();
    delay(500);

  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}
