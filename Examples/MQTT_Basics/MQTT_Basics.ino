#include <PubSubClient.h> //MQTT

#include <ESP8266WiFi.h> //Wifi
#include <ESP8266WebServer.h> //Webserver

#include <ESP8266mDNS.h> //mDNS, for local DNS service "device.local"


const char* ssid = "---";
const char* password = "---";

const char* deviceName = "MQTT Device";

const char* mqtt_server = "192.168.1.68";

const char* state = "off";

WiFiClient espClient;
PubSubClient client(espClient);

ESP8266WebServer server(80);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.hostname(deviceName);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("device")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");

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

  if (String(topic) == "device/on") {

    if (payload_buff == "on") {
      state = "on";
      digitalWrite(LED_BUILTIN, LOW); //inverse of expected, turn on LED
      Serial.println("Turning on LED");
    };

    if (payload_buff == "off") {
      state = "off";
      digitalWrite(LED_BUILTIN, HIGH); //inverse of expected, turn off LED
      Serial.println("Turning off LED");
    };

  } else {
    Serial.println(String(topic));
  }

}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setClient(espClient);
  client.setCallback(callback);
}

//***Pubsub Reconnect Function***
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe("device/on");
      client.subscribe("device/state");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  server.handleClient(); // Listen for HTTP requests from clients
}
