/**
 * Example for reading temperature and humidity
 * using the DHT22 and ESP8266
 *
 * Copyright (c) 2016 Losant IoT. All rights reserved.
 * https://www.losant.com
 */

#include "DHT.h" // Temp/humidity sensor

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <Servo.h>

const char* ssid = "---";
const char* password = "---";

#define DHTPIN 4     // what digital pin the DHT22 is conected to (Pin 4 = D2)
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

Servo servoOne;
int servoVal; //variable to control servo
int servoTimer; //disables stepper if >3000

int timeSinceLastRead = 0;

ESP8266WebServer server(80);   //initiate server at port 80 (http port)
String temperature = ""; //Initialising strings
String humidity = ""; //

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(2000);
  //the HTML of the web page
  
  // Wait for serial to initialize.
  while(!Serial) { }
  WiFi.begin(ssid, password); //begin WiFi connection
  Serial.println("Device Started");
  Serial.println("-------------------------------------");
  Serial.println("Running DHT!");
  Serial.println("-------------------------------------");
   // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("-------------------------------------");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/temp", HTTP_GET, handleGET_Temp);
  server.on("/humid", HTTP_GET, handleGET_Humid);
  server.on("/servo", HTTP_POST, handlePOST_Servo);

  
  server.onNotFound([](){ //Handle undefined addresses.
    server.send(404, "text/plain", "404: Not found");
  });

  
  server.begin();
  Serial.println("Web server started!");
  servoVal = 0;
  servoTimer = 0;
  servoOne.attach(2); //2 = pin D4
}


void loop() {
  server.handleClient();
  // Report every 2 seconds.
  if(timeSinceLastRead > 2000) {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      timeSinceLastRead = 0;
      return;
    }

    // Compute heat index in Fahrenheit (the default)
    float hif = dht.computeHeatIndex(f, h);
    // Compute heat index in Celsius (isFahreheit = false)
    float hic = dht.computeHeatIndex(t, h, false);
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.print(" *C ");
    Serial.print(f);
    Serial.print(" *F\t");
    Serial.print("Heat index: ");
    Serial.print(hic);
    Serial.print(" *C ");
    Serial.print(hif);
    Serial.println(" *F");
    temperature = t;
    humidity = h;
    timeSinceLastRead = 0;
  }

  
  if (servoTimer > 3000) { // Detaches servo after 3s, to stop noise
    servoOne.detach();
  }

  
  delay(100);
  timeSinceLastRead += 100;
  servoTimer += 100;
}

void handleRoot() {
  server.send(200, "text/html", "<h1>Welcome to this Arduino NodeMCU ESP8266</h1><br/><br/><p>Please send HTTP GET requests to /temp, /humid or a POST request with JSON {position: <0-180> to /servo</p>");
}

void handleGET_Temp() {
server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "text/html", temperature);
Serial.println("GET_Temp");
}

void handleGET_Humid() { 
server.sendHeader("Access-Control-Allow-Origin", "*");
server.send(200, "text/html", humidity);
Serial.println("GET_Humidity");

void handlePOST_Servo() { 
server.sendHeader("Access-Control-Allow-Origin", "*");
servoOne.attach(2); //2 = pin D4
String servoVal = server.arg("position"); //Takes JSON sent by a server, reads Position key value
servoOne.write(servoVal.toInt()); //Moves servo to new position
servoTimer = 0;
server.send(200, "text/html", servoVal);
Serial.print("POST_Servo, Position: ");
Serial.println(servoVal);
}
