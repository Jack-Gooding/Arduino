#include "ESP8266WiFi.h"
#include "Env.h"

oid setup() {
  WiFi.begin(ssid, password);  // defined in sketch.h
}

void loop{
  int i = 1;
  dostuff(i);  // defined in mylib
}
