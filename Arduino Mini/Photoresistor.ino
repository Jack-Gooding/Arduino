#include <FastLED.h>

#define LED_PIN     4
#define NUM_LEDS    24
#define BRIGHTNESS  64
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];


#define UPDATES_PER_SECOND 60

void setup() {
  // put your setup code here, to run once:
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );

  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  int sensorValue = analogRead(A0);
  int redPot = analogRead(1);
  int greenPot = analogRead(2);
  int bluePot = analogRead(3);

  int redRGB = map(redPot, 0, 1023, 0, 255);
  int greenRGB = map(greenPot, 0, 1023, 0, 255);
  int blueRGB = map(bluePot, 0, 1023, 0, 255);
  
  float voltage = sensorValue * (5.0 / 1023.0);
  Serial.println(redRGB);

  if (voltage < 0.4) {
    for (int pix = 0; pix < NUM_LEDS; pix++) {
      leds[pix].r = redRGB;
      leds[pix].g = greenRGB;
      leds[pix].b = blueRGB;      
    }
  }
  else
  {
    for (int pix = 0; pix < NUM_LEDS; pix++) {
      leds[pix] = CRGB::Black;
    }
  }
  
  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}
