//#define MQTT_MAX_PACKET_SIZE 256
#include <PubSubClient.h> //MQTT

#include <math.h>

#include <ESP8266WiFi.h> //Wifi
#include <ESP8266WebServer.h> //Webserver

#include <ESP8266mDNS.h> //mDNS, for local DNS service "device.local"

#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);
long lastReconnectAttempt = 0;


ESP8266WebServer server(80);
#include <ESP8266mDNS.h> //mDNS, for local DNS service "device.local"

#include <FastLED.h>

#define LED_PIN     8
#define NUM_LEDS    3
#define BRIGHTNESS  64
#define LED_TYPE    NEOPIXEL
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;



const char* ssid = "---";
const char* password = "---";

const char* deviceName = "Desk Keypad";

const char* mqtt_server = "jack-gooding.com";

byte rows[] = {5,4};
byte cols[] = {0,2,14,12,13};

const int rowCount = sizeof(rows)/sizeof(rows[0]);


const int colCount = sizeof(cols)/sizeof(cols[0]);

int activeCol = 0;

char *buttonNames [rowCount][colCount] = {
  //as many vals as dim1
 {"button1", "button2", "button3", "button4", "button5"},
 {"-","button6","button7","button8", "button9"}//as many rows as dim0
};

byte keys[rowCount][colCount];
byte lastKeys[rowCount][colCount];

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.hostname("Desk Keypad");
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("Desk Keypad")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }


  server.on("/", HTTP_GET, [](){
    Serial.println("Received GET to '/'");
    server.send(200, "text/plain", "This is an IoT device using MQTT protocols. This device is subscribed to 'device/on'");
  });

  server.begin();
}


void callback(char* topic, byte* message, unsigned int length) {
  String payload_buff;
  for (int i=0;i<length;i++) {
    payload_buff = payload_buff+String((char)message[i]);
  }
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(payload_buff); // Print out messages


   if (String(topic) == "keypad/leds") {
    Serial.println(payload_buff);

    StaticJsonDocument<400> doc;
    DeserializationError error = deserializeJson(doc, payload_buff);

    //int redRGB = doc["leds"][0]["red"];

    for (int i = 0; i < NUM_LEDS; i++) {
      int red = doc["leds"][i]["red"];
      int green = doc["leds"][i]["green"];
      int blue = doc["leds"][i]["blue"];
      leds[i].setRGB( red, green, blue);

    };
    FastLED.show();
   };





}


void allPinsLow() {
  for (int i = 0; i < colCount; i++) {
    digitalWrite(cols[i], LOW);
  }
};

void setup() {
  // put your setup code here, to run once:
  delay( 1000 ); // power-up safety delay
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
     leds[i].setRGB( 255, 68, 221);
  }
  FastLED.show();

  for (int i = 0; i < rowCount; i++) {
    pinMode(rows[i], INPUT);
  }

  for (int i = 0; i < colCount; i++) {
    pinMode(cols[i], OUTPUT);
  }

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setClient(espClient);
  client.setCallback(callback);
}



//***Pubsub Reconnect Function***
boolean reconnect() {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceName)) {
      Serial.println("connected");
      client.subscribe("device/on");
      client.subscribe("device/state");
      client.subscribe("keypad/leds");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
    }
    return client.connected();
}

void sendMQTT(char* topic) {
    client.publish(topic, topic);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
  client.loop();
  }

  server.handleClient(); // Listen for HTTP requests from clients

  switch (activeCol) {
  case 0:
    allPinsLow();
    digitalWrite(cols[activeCol], HIGH);
    break;
  case 1:
    allPinsLow();
    digitalWrite(cols[activeCol], HIGH);
    break;
  case 2:
    allPinsLow();
    digitalWrite(cols[activeCol], HIGH);
    break;
  case 3:
    allPinsLow();
    digitalWrite(cols[activeCol], HIGH);
    break;
  case 4:
    allPinsLow();
    digitalWrite(cols[activeCol], HIGH);
    break;
  default:
    allPinsLow();
    break;
}



  for (int i = 0; i < rowCount; i++) {
    int rowVal = digitalRead(rows[i]);
    if (rowVal != lastKeys[i][activeCol]) {

      if (rowVal == HIGH) {
       //Serial.print("Key Pressed: Row ");
       //Serial.print(i+1);
       //Serial.print(": Col ");
       //Serial.println(activeCol+1);

       client.publish("keypad/button/pressed", buttonNames[i][activeCol]);

      }

      if (rowVal == LOW) {
       //Serial.print("Key Released: Row ");
       //Serial.print(i+1);
       //Serial.print(": Col ");
       //Serial.println(activeCol+1);
       client.publish("keypad/button/released", buttonNames[i][activeCol]);
      }
    }
    lastKeys[i][activeCol] = rowVal;
  }

  activeCol++;
  if (activeCol > 4) {
    activeCol = 0;
  }
  FastLED.show();
  delay(1);
}
