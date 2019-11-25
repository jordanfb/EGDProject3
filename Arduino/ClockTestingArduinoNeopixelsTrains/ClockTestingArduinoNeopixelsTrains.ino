#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 6
#define INNER_PIN 5
#define NUMBER_OF_PINS 300
#define NUMBER_OF_PINS_INNER 60
#define CLOCK_PINS 60
#define TRAINS_PER_PLAYER 1

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_PINS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel insideStrip = Adafruit_NeoPixel(NUMBER_OF_PINS_INNER, INNER_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel clockStrip = Adafruit_NeoPixel(CLOCK_PINS, PIN, NEO_GRB + NEO_KHZ800);

// variables
int delayTime = 50;
int nodesPerSector = NUMBER_OF_PINS/5;
int trainStation = 0;
int currentClockPins = CLOCK_PINS - 1;
int timer = 0;

// Player colors, color number corresponds to player number, feel free to change these color values here
uint32_t color1 = strip.Color(255, 0, 0);
uint32_t color2 = strip.Color(0, 0, 255);
uint32_t color3 = strip.Color(0, 255, 0);
uint32_t color4 = strip.Color(255, 0, 255);
uint32_t color5 = strip.Color(255, 255, 0);
uint32_t baseColor = strip.Color(25, 25, 25);

// arrays for each info for a players trains
int playerVotes[5 * TRAINS_PER_PLAYER];
int trainsID[5 * TRAINS_PER_PLAYER];
int trainsStopped[5 * TRAINS_PER_PLAYER];
int location[5 * TRAINS_PER_PLAYER];
uint32_t headColors[5 * TRAINS_PER_PLAYER];
uint32_t tailColors[5 * TRAINS_PER_PLAYER];

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  // End of trinket special code

//  strip.begin();
//  strip.setBrightness(60);
//  strip.show(); // Initialize all pixels to 'off'
//  Serial.begin(9600);
//  Serial.setTimeout(200);
//
//  UpdateStripColor(baseColor, strip, NUMBER_OF_PINS);

  trainStation = (nodesPerSector / 2) - 2;

  strip.begin();
  strip.setBrightness(60);
  strip.show();
  Serial.begin(9600);
  Serial.setTimeout(200);

  UpdateStripColor(color1, strip, CLOCK_PINS);
  timer = millis();
}

void loop() {
//  // loop through all possible trains and run the functionality they should have at that time
//  for(int i=0; i<(5*TRAINS_PER_PLAYER); i++) {
//    if(trainsID[i] != 0 && !trainsStopped[i]) { // if there is a spawned train and its not stopped
//      // make sure location is only a value between 1 and NUMBER_OF_PINS
//      if(location[i] >= NUMBER_OF_PINS) { location[i] %= NUMBER_OF_PINS; }
//      
//      location[i] += 1; // move the train forward one pin
//      drawTrain(location[i], headColors[i], tailColors[i]); // draw the train on the strip
//      strip.show();
//      delay(delayTime);
//    } else if (trainsStopped[i]) { // if train is stopped
//      drawTrain(location[i], headColors[i], tailColors[i]); // draw the train at the stopped station
//      strip.show();
//      delay(delayTime);
//    }
//  }

  if(millis() - timer >= 1000) {
    ResetPin(currentClockPins, strip);
    UpdateStripColor(baseColor, strip, CLOCK_PINS);
    currentClockPins--;
    UpdateStripColor(color1, strip, currentClockPins);
    timer = millis();
    strip.show();
    delay(1);
  }

  // code for reading in serial inputs/calls
  if (Serial.peek() != -1) {
    char senderID; // ID
    char receiverID; // ID
    char hijackerID; // ID
    char trainID; // ID
    char questionStrip; // boolean
    char newSpeed; // int
    char candidateA; // ID
    char candidateB; // ID
    
    char c = Serial.read();
    switch(c) {
      case 'a': // "Who are you?" case
Serial.print("WHO ARE YOU?:\n");
        // send message to hub ... I am ____
        Serial.write('r');
        Serial.write('6');
        break;
      case 'b': // create a train
Serial.print("CREATE TRAIN:\n");
        senderID = Serial.read();
        Serial.print(senderID + "\n");
        receiverID = Serial.read();
        Serial.print(receiverID + "\n");
        trainID = Serial.read();
        Serial.print(trainID + "\n");
        
        SpawnTrain(senderID, receiverID, trainID);
        break;
      case 'c': // pause a train
Serial.print("PAUSE TRAIN:\n");
        trainID = Serial.read();
        hijackerID = Serial.read();
        
        PauseTrain(hijackerID, trainID);
        break;
      case 'd': // release a train
Serial.print("RELEASE TRAIN:\n");
        trainID = Serial.read();
        
        ReleaseTrain(trainID);
        break;
      case 'e': // answer a train
Serial.print("ANSWER TRAIN:\n");
        trainID = Serial.read();
        
        AnswerTrain(trainID);
        break;
      case 'f': // destroy a train
Serial.print("DESTROY TRAIN:\n");
        trainID = Serial.read();

        DestroyTrain(trainID);
        break;
      case 'i': // set speed of lights
Serial.print("SETTING SPEED:\n");
        questionStrip = Serial.read();
        newSpeed = Serial.read();

        SetSpeed(newSpeed, questionStrip);
        break;
      case 'j': // send player votes in
Serial.print("CASTING VOTES:\n");
        senderID = Serial.read();
        candidateA = Serial.read();
        candidateB = Serial.read();

        Vote(senderID, candidateA, candidateB);
        break;
      case 'k': // new game
Serial.print("RESET GAME:\n");
        ResetGame();
        break;
    }
  }
}

// function to visually draw the lights of a train
void drawTrain(int i, uint32_t head, uint32_t tail) {
    strip.setPixelColor((i+149)%NUMBER_OF_PINS, 20, 20, 20);
    strip.setPixelColor((i)%NUMBER_OF_PINS, tail);
    strip.setPixelColor((i+1)%NUMBER_OF_PINS, tail);
    strip.setPixelColor((i+2)%NUMBER_OF_PINS, tail);
    strip.setPixelColor((i+3)%NUMBER_OF_PINS, head);
}

// function to spawn a new train
void SpawnTrain(char sender, char receiver, char trainID) {
//  Serial.print("Spawning a train at Player " + sender + " and sending to Player " + receiver + "with trainID = " + trainID + "\n");
  uint32_t fromColor = GetColor(sender);
  uint32_t toColor = GetColor(receiver);

  int playerNumber = (ToInt(sender) - 1) * TRAINS_PER_PLAYER;
  int startPosition = (trainStation + (nodesPerSector * playerNumber));
  drawTrain(startPosition, fromColor, toColor);
  strip.show();

  // update arrays for the new train that was just spawned
  for(int i=0; i<TRAINS_PER_PLAYER; i++) {
    if(trainsID[playerNumber + i] == 0) {    
      location[playerNumber + i] = startPosition;
      headColors[playerNumber + i] = fromColor;
      tailColors[playerNumber + i] = toColor;
      trainsID[playerNumber + i] = ToInt(trainID);
      return;
    }
  }

  Serial.print("COULD NOT SPAWN TRAIN!\n");
}

// function to destroy a train
void DestroyTrain(char trainID) {
  //Serial.print("Destroying a train with trainID = " + trainID + "\n");
  int trainIndex = GetTrainIndex(ToInt(trainID));

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex], strip);
  ResetPin(location[trainIndex] + 1, strip);
  ResetPin(location[trainIndex] + 2, strip);
  ResetPin(location[trainIndex] + 3, strip);
  strip.show();

  location[trainIndex] = 0;
  headColors[trainIndex] = 0;
  tailColors[trainIndex] = 0;
  trainsStopped[trainIndex] = 0;
  trainsID[trainIndex] = 0;
}

// function to pause a train
void PauseTrain(char hijacker, char trainID) {
  //Serial.print("Paused a train at Player " + hijacker + " that had trainID = " + trainID + "\n");
  int hijackerNumber = ToInt(hijacker) - 1;
  int trainIndex = GetTrainIndex(ToInt(trainID));

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex], strip);
  ResetPin(location[trainIndex] + 1, strip);
  ResetPin(location[trainIndex] + 2, strip);
  ResetPin(location[trainIndex] + 3, strip);
  strip.show();

  // set location to the station of the player who hijacked it
  location[trainIndex] = (trainStation + (nodesPerSector * hijackerNumber));
  trainsStopped[trainIndex] = 1;
}

// function to release a paused train
void ReleaseTrain(char trainID) {
  //Serial.print("Released a train paused with trainID = " + trainID + "\n");
  int trainIndex = GetTrainIndex(ToInt(trainID));

  // need to set location to the players station
  trainsStopped[trainIndex] = 0;
}

// function to get the color of a player id
uint32_t GetColor(char id) {
  if(id == '1') {
    return color1;
  } else if(id == '2') {
    return color2;
  } else if(id == '3') {
    return color3;
  } else if(id == '4') {
    return color4;
  } else if(id == '5') {
    return color5;
  }
}

// function to change the speed that lights are delayed by
void SetSpeed(char newSpeed, char questionStrip) {
  if(ToInt(questionStrip) == 0) {
    Serial.print("question strip was 0");
  } else if(ToInt(questionStrip) == 1) {
    Serial.print("question strip was 1");
  }
  //set to time between station
  delayTime = ToInt(newSpeed);
}

// function to update the strip to a single color
void UpdateStripColor(uint32_t color, Adafruit_NeoPixel strip_, int stripPins) {
  for(int i=0; i<stripPins; i++) {
    strip_.setPixelColor(i, color);
  }

  strip_.show();
}

// function to reset a pin to the base color of the strip
void ResetPin(int pin, Adafruit_NeoPixel strip_) {
  strip_.setPixelColor(pin, baseColor);
}

// function to reset the game for a new game
void ResetGame() {
  UpdateStripColor(baseColor, strip, NUMBER_OF_PINS); // update strip to the base color

  // reset all arrays that store train information
  for(int i=0; i<5 * TRAINS_PER_PLAYER; i++) {
    trainsID[i] = 0;
    trainsStopped[i] = false;
    location[i] = 0;
    headColors[i] = 0;
    tailColors[i] = 0;
  }
}

// function to answer a train and move it to the answered strip
void AnswerTrain(char trainID) {
  // DstroyTrain(trianID)
  // maybe have new array to hold inner values
    // OR have a bool array for if the train was answered and then populate information and add a check for inner loop and put on that strip
}

// function to set who a player is voting for
void Vote(char senderID, char vote1, char vote2) {
  int playerNumber = ToInt(senderID) - 1;
  int voteIndex = playerNumber * 2;

  playerVotes[voteIndex] = ToInt(vote1);
  playerVotes[voteIndex + 1] = ToInt(vote2);
}

// helper function to turn a trainID into its corresponding index value
int GetTrainIndex(int value) {
  for(int i=0; i<5 * TRAINS_PER_PLAYER; i++) {
    if(trainsID[i] == value) {
      return i;
    }
  }

  return -1;
}

// helper function to convert a char into an int
int ToInt(char c) {
  int ret = c - '0';
  return ret;
}

// function to adjust the current length of the clock
void SetClockLength(int len) {
  
}

// function to adjust how long the clock will take to go through
void SetClockTime(int time_) {
  
}
