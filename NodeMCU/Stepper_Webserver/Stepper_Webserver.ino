

// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain




#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>        // Include the mDNS library

#include <ArduinoJson.h>

const char* wifi_ssid = "---";
const char* wifi_passwd = "---";
MDNSResponder mdns;

// A4988 driver test routine
#define ENABLE 2
#define STEP 4
#define DIR 5

#define STEPS_PER_ROTATION 200

#define HTTP_REST_PORT 80 //default HTTP port
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

void POST_stepper() { 
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
  digitalWrite(ENABLE, LOW); //turns on stepper driver
  
  int steps = JObject["steps"]; //variables from incoming JSON
  String rotation = JObject["direction"];
  
  if (rotation == "clockwise") {
      digitalWrite(DIR,HIGH);
  } else {
      digitalWrite(DIR,LOW);
  }
   
  for (int i = 0; i < steps; i+=200) { //step counts >~500 were causing timeout watchdog errors, this breaks steps into "blocks" of 1-200 steps to prevent thisthrough until done. //This is a hack
    Serial.println(i);
    if ((steps-i) < 201) { //if steps > 200, break into blocks of 200, if less, do steps as normal
        //Serial.println(steps-i);
      for( int j=0;j<(steps-i);++j) {
        //Serial.print(j);
        digitalWrite(STEP,HIGH);
        delayMicroseconds(700);
        digitalWrite(STEP,LOW);
        delayMicroseconds(700);
      }
    } else {
      //Serial.println(200);
      for(int j=0;j<200;++j) {
      //Serial.print(j);      
        digitalWrite(STEP,HIGH);
        delayMicroseconds(700);
        digitalWrite(STEP,LOW);
        delayMicroseconds(700);
      }
    }
    yield();
  }
  digitalWrite(ENABLE, HIGH); //turns off stepper driver when done
}


void handleNotFound() {
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



void stepNow(int totalSteps) { //Not used but useful for troubleshooting
  //Serial.print(totalSteps);
  //Serial.println(F(" steps."));

  int i;
  for(i=0;i<totalSteps;++i) {
    digitalWrite(STEP,HIGH);
    delayMicroseconds(700);
    digitalWrite(STEP,LOW);
    delayMicroseconds(700);
  }
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

void walkBothDirections() {
  //Serial.println(F("dir LOW"));
  digitalWrite(DIR,LOW);
  stepNow(STEPS_PER_ROTATION);
  
  //Serial.println(F("dir HIGH"));
  digitalWrite(DIR,HIGH);
  stepNow(STEPS_PER_ROTATION);
}

void doStepper() {
  //Serial.println(F("Enable HIGH"));
  digitalWrite(ENABLE,HIGH);
  walkBothDirections();
  //Serial.println(F("Enable LOW"));
  digitalWrite(ENABLE,LOW);
  walkBothDirections();
  http_rest_server.send(200, "text/plain", "success");
}

void config_rest_server_routing() {
    http_rest_server.on("/stepper", HTTP_OPTIONS, []() {
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
            "Welcome to the ESP8266 REST Web Server!");
    });
    http_rest_server.on("/stepper", HTTP_GET, doStepper);
    http_rest_server.on("/stepper", HTTP_POST, POST_stepper);
    http_rest_server.onNotFound(handleNotFound);
    

}

void setup() {
  Serial.begin(9600);
  
  pinMode(ENABLE,OUTPUT); //Set GPIO pins to output
  pinMode(STEP,OUTPUT);
  pinMode(DIR,OUTPUT);
  
  digitalWrite(ENABLE, HIGH); //ENABLE pin controlls A4988 stepper driver, high enable prevents stepper being held in place constantly.

  if (init_wifi() == WL_CONNECTED) { // confirms connection
        Serial.print("Connetted to ");
        Serial.print(wifi_ssid);
        Serial.print("--- IP: ");
        Serial.println(WiFi.localIP());
        if (mdns.begin("ws2812b", WiFi.localIP())) {
        Serial.println("MDNS responder started");
}
    }
    else {
        Serial.print("Error connecting to: ");
        Serial.println(wifi_ssid);
  }
  
  config_rest_server_routing(); // configures routing for REST responses

  http_rest_server.begin();
  Serial.println("HTTP REST Server Started");
}

void loop() {
  http_rest_server.handleClient();
  // Wait a few seconds between measurements.
}
