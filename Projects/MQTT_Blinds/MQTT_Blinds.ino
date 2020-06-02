//https://github.com/kakopappa/arduino-esp8266-alexa-wemo-switch/blob/master/sinric.ino

#include <EEPROM.h>
#include <PubSubClient.h> //MQTT
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>


#include <AccelStepper.h>

#define enablePin     2
const int stepPin = 4;
const int dirPin = 5;

/* EEPROM Data Layout
 * 0: Steps for full rollout
 * 1: Current number of steps
 * 2: Max Speed
 * 3: Accelleration
 * 4: Initial Speed
 * 5: Index of current steps
 * 6-12: Initialisation trackers 255 = uninitialised,
 */


int blindsFullDeploySteps = 95000; //95,000 is ~full length
int adjustSteps = EEPROM.read(0);
int blindsCurrentDeployment = EEPROM.read(1);

//int MaxSpeed = EEPROM.read(2); //600 is good
//int Acceleration = EEPROM.read(3); //30 is good
//int Speed = EEPROM.read(4); //20 is good

int MaxSpeed = 600;
int Acceleration = 30;
int Speed = 20;

int a;
int b;
int c;


WiFiClient espClient;
PubSubClient client(espClient);


String location = "Blinds";
AccelStepper stepper1(1, stepPin, dirPin);
// prototypes
boolean connectWifi();

// Change this!!
const char* ssid = "---";
const char* password = "---";

boolean wifiConnected = false;

ESP8266WebServer server(80);

void POST_blinds() {
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
  StaticJsonDocument<200> JObject; //creates a memory slot for incoming JSON, if data gets cut off or fails, increase this.
  //JsonObject JObject = jsonBuffer.parseObject(server.arg("plain")); //creates object from parsed data held in .arg("plain").
  DeserializationError error = deserializeJson(JObject, server.arg("plain"));
  if (error)
  return;
  int moveDistance = JObject["moveDistance"];

  int newMaxSpeed = JObject["maxSpeed"];
  int newAcceleration = JObject["Acceleration"];
  int newSpeed = JObject["Speed"];
  Serial.println(newMaxSpeed);
  Serial.println(newAcceleration);
  Serial.println(newSpeed);
  if (newMaxSpeed)
  {
    EEPROM.put(2, newMaxSpeed);
    stepper1.setMaxSpeed(newMaxSpeed);
    MaxSpeed = newMaxSpeed;
  }
  if (newAcceleration)
  {
    EEPROM.put(3, newAcceleration);
    stepper1.setMaxSpeed(newAcceleration);
    Acceleration = newAcceleration;
  }
  if (newSpeed)
  {
     EEPROM.put(4, newSpeed);
    stepper1.setMaxSpeed(newSpeed);
    Speed = newSpeed;
  }

  stepper1.moveTo(moveDistance);
  blindsCurrentDeployment = moveDistance;

}




void GET_blinds() {
  //https://arduinojson.org/v5/example/http-server/ follow this to understand JSON
    StaticJsonDocument<200> JObject;
    char JSONmessageBuffer[200];
          JObject["maxDeploymentSteps"] = blindsFullDeploySteps;
          JObject["deploymentSteps"] = EEPROM.read(1);
          JObject["MaxSpeed"] = MaxSpeed;
          JObject["Acceleration"] = Acceleration;
          JObject["Speed"] = Speed;

        serializeJson(JObject, JSONmessageBuffer);
        Serial.println(JSONmessageBuffer);
        //http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");
        //http_rest_server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
        //http_rest_server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");

        server.send(200, "application/json", JSONmessageBuffer);
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


   if (String(topic) == "bedroom/blinds") {
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload_buff);
    int value = doc["value"];
    int maxDeploymentSteps = doc["max"];
    int moveDistance = doc["steps"];
    int deploymentSteps = doc["steps"];
    int MaxSpeed = doc["mSpeed"];
    int Acceleration = doc["acc"];
    String dir = doc["direction"];
    if (dir == "up") {
           stepper1.move(moveDistance);
           blindsCurrentDeployment = moveDistance;
    } else {
          stepper1.move(-moveDistance);
          blindsCurrentDeployment = moveDistance;
    };
    Serial.println(value);

   };

}


void setup() {
  Serial.begin(9600);
  stepper1.setMaxSpeed(MaxSpeed);
  stepper1.setAcceleration(Acceleration);
  stepper1.setSpeed(Speed);
  delay(500);
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);
  // Initialise wifi connection
  wifiConnected = connectWifi();
a = 600;
b = 30;
c = 20;

EEPROM.put(2, a);
EEPROM.put(3, b);
EEPROM.put(4, c);


  if(wifiConnected){
    server.on("/", HTTP_GET, [](){
    server.send(200, "text/plain", "This is an example index page your server may send.");
    });
    server.on("/blinds", HTTP_OPTIONS, []() {
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
    server.on("/blinds", HTTP_POST, POST_blinds);
    server.on("/blinds", HTTP_GET, GET_blinds);

    server.onNotFound([](){
        server.send(404, "text/plain", "Not found");
    });

    client.setServer("jack-gooding.com",1883);
    client.setCallback(callback);
    server.begin(); //omit this since it will be done by espalexa.begin(&server)
  } else
  {
    delay(500);
    while (1)
    {

      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }
}


void loop() {
  delay(1);
  stepper1.run();
  client.loop();
  server.handleClient(); // Listen for HTTP requests from clients
  if (stepper1.distanceToGo() == 0) {
    digitalWrite(enablePin, HIGH);
  } else {
    digitalWrite(enablePin, LOW);
  }
  if (client.connected()) {
      client.loop();
   } else {
    connectMQTT();
   }
}

boolean connectMQTT() {
  if (client.connect("Bedroom Blinds")) {
    client.subscribe("bedroom/blinds");
    client.publish("bedroom/blinds", "test");
  };
}


void firstDeviceChanged(uint8_t brightness) {
    Serial.print("Device 1 changed to ");

    //do what you need to do here

    //EXAMPLE
    if (brightness == 255) {
      Serial.println("ON");
      stepper1.moveTo(-blindsFullDeploySteps);
      blindsCurrentDeployment = -blindsFullDeploySteps;
    }
    else if (brightness == 0) {
      Serial.println("OFF");
      stepper1.moveTo(0);
      blindsCurrentDeployment = 0;
    }
    else {
      Serial.print("DIM "); Serial.println(brightness);
      int distanceToMove = map(brightness, 0, 255, 0, -blindsFullDeploySteps);
      stepper1.moveTo(distanceToMove);
      blindsCurrentDeployment = distanceToMove;
    }

}


boolean connectWifi(){
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.hostname("Desk Keypad");
  WiFi.begin(ssid, password);

  // Wait for connection
  Serial.println("");
  Serial.print("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 50){
      state = false;
      break;
    }
    i++;
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}
