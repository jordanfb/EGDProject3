#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(150, PIN, NEO_GRB + NEO_KHZ800);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

int delayTime = 50;
int nodesPerSector = 30;
int trainStation = 0;

// Player colors, color number corresponds to player number, feel free to change these color values here
uint32_t color1 = strip.Color(255, 0, 0);
uint32_t color2 = strip.Color(0, 0, 255);
uint32_t color3 = strip.Color(0, 255, 0);
uint32_t color4 = strip.Color(255, 255, 0);
uint32_t color5 = strip.Color(255, 0, 255);
uint32_t baseColor = strip.Color(25, 25, 25);


// arrays for each info for a players trains
bool spawnedTrains[] = {false, false, false, false, false};
bool trainsStopped[] = {false, false, false, false, false};
bool needsClear[] = {false, false, false, false, false};
int location[] = {0, 0, 0, 0, 0};
uint32_t headColors[] = {0, 0, 0, 0, 0};
uint32_t tailColors[] = {0, 0, 0, 0, 0};

bool spawned = false;
int spawnLocation;
uint32_t spawnedColor;
uint32_t spawnedColorTail;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

  strip.begin();
  strip.setBrightness(60);
  strip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  Serial.setTimeout(200);

  UpdateStripColor(baseColor);

  trainStation = (nodesPerSector / 2) - 2;
}

void loop() {
  // Some example procedures showing how to display to the pixels:
//  for(int i =0; i < 150; i++) {
//    drawTrain(i, strip.Color(0, 255, 0), strip.Color(255, 0, 0));
//    drawTrain(i+70, strip.Color(0, 0, 255), strip.Color(255, 255, 0));
//    drawTrain(i+120, strip.Color(0, 0, 255), strip.Color(255, 0, 255));
//    drawTrain(i+100, strip.Color(0, 0, 255), strip.Color(100, 100, 255));
//    strip.show();
//    delay(delayTime);
//  }

  for(int i=0; i<5; i++) {
    if(spawnedTrains[i] && !trainsStopped[i]) {
      if(location[i] >= 150) {
        Serial.print(location[i] + '\n');
        location[i] %= 150;
        Serial.print(location[i] + '\n');
      }
        
      location[i] += 1;
      drawTrain(location[i], headColors[i], tailColors[i]);
      strip.show();
      delay(delayTime);
    } else if (trainsStopped[i]) {
      drawTrain(location[i], headColors[i], tailColors[i]);
      strip.show();
      delay(delayTime);
    }
  }

//  drawTrain(trainStation, strip.Color(0, 255, 0), strip.Color(255, 0, 0));
//  drawTrain(30 + trainStation, strip.Color(0, 0, 255), strip.Color(255, 255, 0));
//  drawTrain(60 + trainStation, strip.Color(0, 0, 255), strip.Color(255, 0, 255));
//  drawTrain(90 + trainStation, strip.Color(0, 0, 255), strip.Color(100, 100, 255));
  if (Serial.peek() != -1) {
    // code to read in a string from serial input and break it into pieces to call a function
    String data = Serial.readString();
    data.toLowerCase();


    if(data.substring(0,9) == "set speed") {
      SetSpeed(data.substring(9).toInt());
      Serial.print(data.substring(2).toInt());
    } else {
      String function = data.substring(0, 1);
      String id1 = data.substring(1, 2);
      String id2 = data.substring(2, 3);
      
      if(function == "s") {
        Serial.print("Spawning a train at Player " + id1 + " and sending to Player " + id2 + "\n");
        SpawnTrain(id1, id2);
      } else if(function == "d") {
        Serial.print("Destroying a train sent from Player " + id1 + " and received by Player " + id2 + "\n");
        //DestroyTrain(id1, id2);
      } else if(function == "p") {
        Serial.print("Paused a train at Player " + id1 + " that was sent by Player " + id2 + "\n");
        Pause(id1, id2);
      } else if(function == "r") {
        Serial.print("Resumed a train paused by Player " + id1 + " that was sent by Player " + id2 + "\n");
        Resume(id1, id2);
      }
    }

    data = "";

//    char c = Serial.read();
//    Serial.print(c);
//    if(c == 'a') {
//      Serial.print("Reading strings...");
//      String t1 = Serial.readString();
//      String t2 = Serial.readString();
//      String t3 = Serial.readString();
//      Serial.print(t1 + " and " + t2 + " and " + t3);
//    }

    // for changing speed through input
    /*int newSpeed = s.toInt();
    if (newSpeed > 0) {
      delayTime = newSpeed;
    }*/
  }
}

void drawTrain(int i, uint32_t head, uint32_t tail) {
    strip.setPixelColor((i+149)%150, 20, 20, 20);
    strip.setPixelColor((i)%150, tail);
    strip.setPixelColor((i+1)%150, tail);
    strip.setPixelColor((i+2)%150, tail);
    strip.setPixelColor((i+3)%150, head);
}

void SpawnTrain(String from, String to) {
  uint32_t fromColor = GetColor(from);
  uint32_t toColor = GetColor(to);

  int playerNumber = from.toInt() - 1;
  int startPosition = (trainStation + (30 * playerNumber));
  drawTrain(startPosition, fromColor, toColor);
  strip.show();

  location[playerNumber] = startPosition;
  headColors[playerNumber] = fromColor;
  tailColors[playerNumber] = toColor;
  spawnedTrains[playerNumber] = true;
}


void Pause(String hijacker, String sender) {
  int hijackerNumber = hijacker.toInt() - 1;
  int senderNumber = sender.toInt() - 1;

Serial.print(location[senderNumber]);
  ResetPin(location[senderNumber]);
  ResetPin(location[senderNumber] + 1);
  ResetPin(location[senderNumber] + 2);
  ResetPin(location[senderNumber] + 3);
  strip.show();

  // need to set location to the players station
  location[senderNumber] = (trainStation + (30 * hijackerNumber));
  trainsStopped[senderNumber] = true;
//  needsClear[senderNumber] = true;

Serial.print(location[senderNumber]);
}

void Resume(String hijacker, String sender) {
  int hijackerNumber = hijacker.toInt() - 1;
  int senderNumber = sender.toInt() - 1;

  // need to set location to the players station
  //location[senderNumber] = (trainStation + (30 * hijackerNumber));
  trainsStopped[senderNumber] = false;
}

uint32_t GetColor(String id) {
  if(id == "1") {
    return color1;
  } else if(id == "2") {
    return color2;
  } else if(id == "3") {
    return color3;
  } else if(id == "4") {
    return color4;
  } else if(id == "5") {
    return color5;
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void SetSpeed(int spd) {
  //set to time between station
  delayTime = spd;
}

void UpdateStripColor(uint32_t color) {
  for(int i=0; i<150; i++) {
    strip.setPixelColor(i, color);
  }

  strip.show();
    delay(delayTime);
}

void ResetPin(int pin) {
  strip.setPixelColor(pin, baseColor);
//  strip.show();
//  delay(5);
}
