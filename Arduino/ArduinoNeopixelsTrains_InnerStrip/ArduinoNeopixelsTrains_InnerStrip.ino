#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6
#define INNER_PIN 5
#define NUMBER_OF_PINS_OUTER 100
#define NUMBER_OF_PINS_INNER 90
#define TRAINS_PER_PLAYER 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel outerStrip = Adafruit_NeoPixel(NUMBER_OF_PINS_OUTER + 20, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel innerStrip = Adafruit_NeoPixel(NUMBER_OF_PINS_INNER, INNER_PIN, NEO_GRB + NEO_KHZ800);

// variables
int delayTime = 20; // LEDS * delayTime / 1000 = seconds
int nodesPerSector = NUMBER_OF_PINS_OUTER / 5;
int nodesPerInnerSector = NUMBER_OF_PINS_INNER / 5;
int trainStation = 0;
int innerStation = 1;
unsigned long timeHolder = 0;
int clockTick = 750;
int currentClockPins = 59;
int loopCount = 0;
int votingBlinkLoops = 0;
bool votingBlink = false;

// Player colors, color number corresponds to player number, feel free to change these color values here
uint32_t color1 = outerStrip.Color(255, 0, 0);
uint32_t color2 = outerStrip.Color(0, 0, 255);
uint32_t color3 = outerStrip.Color(0, 255, 0);
uint32_t color4 = outerStrip.Color(255, 0, 255);
uint32_t color5 = outerStrip.Color(255, 255, 0);
uint32_t baseColor = outerStrip.Color(25, 25, 25);
uint32_t blankColor = outerStrip.Color(0, 0, 0);
uint32_t voteBlinkColor = outerStrip.Color(183, 28, 28);
uint32_t allColors[] = {color1, color2, color3, color4, color5, blankColor};
int tester = 1;

// arrays for each info for a players trains
uint32_t playerVotes[10] = {blankColor, blankColor, blankColor, blankColor, blankColor, blankColor, blankColor, blankColor, blankColor, blankColor};
int trainsID[5 * TRAINS_PER_PLAYER];
int trainsStopped[5 * TRAINS_PER_PLAYER];
int answeredTrains[5 * TRAINS_PER_PLAYER];
int location[5 * TRAINS_PER_PLAYER];
uint32_t headColors[5 * TRAINS_PER_PLAYER];
uint32_t tailColors[5 * TRAINS_PER_PLAYER];

bool startTest = true;

// variables for receiving information from Hub
byte buffer_[5];
bool messageReady = false;
int currentByte = 0;

int testingVoteBlink = 0;

void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
  // End of trinket special code

  outerStrip.begin();
  outerStrip.setBrightness(40);
  outerStrip.show(); // Initialize all pixels to 'off'
  innerStrip.begin();
  innerStrip.setBrightness(40);
  innerStrip.show(); // Initialize all pixels to 'off'
  Serial.begin(9600);
  Serial.setTimeout(200);

  UpdateStripColor(baseColor);
  UpdateInnerStripColor(baseColor);
  for(int i=110; i<120; i++) {
    outerStrip.setPixelColor(i, baseColor);
  }

  //trainStation = (nodesPerSector / 2) - 2;
  //innerStation = (nodesPerInnerSector / 2);

  timeHolder = millis();
}

void loop() {
  // loop through all possible trains and run the functionality they should have at that time
  for (int i = 0; i < (5 * TRAINS_PER_PLAYER); i++) {
    if (trainsID[i] != 0 && !trainsStopped[i] && !answeredTrains[i]) { // if there is a spawned train and its not stopped
      location[i] += 1; // move the train forward one pin
      if (location[i] >= NUMBER_OF_PINS_OUTER) { location[i] %= NUMBER_OF_PINS_OUTER; }

      drawTrain(location[i], headColors[i], tailColors[i]); // draw the train on the strip
//      outerStrip.show();
    } else if (trainsStopped[i] && !answeredTrains[i]) { // if train is stopped and not on answer track
      drawTrain(location[i], headColors[i], tailColors[i]); // draw the train at the stopped station
//      outerStrip.show();
    } else if (trainsStopped[i] && answeredTrains[i]) { // if train is stopped and on answer track
      drawTrainInner(location[i], headColors[i]);
//      innerStrip.show();
    } else if (answeredTrains[i]) {
      location[i] += 1;
      if (location[i] >= NUMBER_OF_PINS_INNER) { location[i] %= NUMBER_OF_PINS_INNER; }
      
      drawTrainInner(location[i], headColors[i]);
//      innerStrip.show();
    }
  }

  for(int i=0; i<5; i++) {
    GenerateVoteColors(i, playerVotes[i * 2], playerVotes[(i * 2) + 1]);
  }

  outerStrip.show();
  innerStrip.show();

  delay(delayTime);

// handle incoming serial messages
  if (Serial.peek() != -1) {
    char currentRead = Serial.read();
    messageReady = false;

    if (currentRead == '\0' || currentRead == '\n') {
      currentByte = 0;
      messageReady = true;
    } else {
      buffer_[currentByte] = currentRead;
      currentByte++;
    }
  }

// check if a full serial message has been sent and is ready to call a function
  if (messageReady) {
    switch (buffer_[0]) {
      case (byte)'a': // "Who are you?" case
        SendDebugMessage("WHO ARE YOU?:\n");
        Serial.write('r');
        Serial.write((byte)6);
        Serial.write('\0');
        Serial.write('i');
        Serial.write((byte)delayTime);
        Serial.write('\0');
        break;
      case (byte)'b': // create a train
        SendDebugMessage("CREATE TRAIN:\n");
        SpawnTrain(buffer_[1], buffer_[2], buffer_[3]);
        break;
      case 'c': // pause a train
        SendDebugMessage("PAUSE TRAIN:\n");
        PauseTrain(buffer_[1], buffer_[2]);
        break;
      case 'd': // release a train
        SendDebugMessage("RELEASE TRAIN:\n");
        ReleaseTrain(buffer_[1]);
        break;
      case 'e': // answer a train
        SendDebugMessage("ANSWER TRAIN:\n");
        AnswerTrain(buffer_[1]);
        break;
      case 'f': // destroy a train
        SendDebugMessage("DESTROY TRAIN:\n");
        DestroyTrain(buffer_[1]);
        break;
      case 'i': // set speed of lights
        SendDebugMessage("SETTING SPEED:\n");
        SetSpeed(buffer_[1]);
        break;
      case 'j': // send player votes in
        SendDebugMessage("CASTING VOTES:\n");
        SendDebugMessage(buffer_[1]);
        SendDebugMessage(buffer_[2]);
        SendDebugMessage(buffer_[3]);
        Vote(buffer_[1], buffer_[2], buffer_[3]);
        break;
      case 'k': // new game
        SendDebugMessage("RESET GAME:\n");
        ResetGame();
        break;
      case 't': // resync timer
        loopCount = 0;
        Serial.write('u');
        Serial.write('\0');
        break;
      default:
        SendDebugMessage("WTF");
        SendDebugMessage(buffer_[0]);
        SendDebugMessage((char)buffer_[0]);
        break;
    }

    messageReady = false;
  }

  // CLOCK TESTING STUFF
  /*if(millis() - timeHolder >= clockTick) {
    ResetPin(currentClockPins);
    currentClockPins--;
    strip.show();
    timeHolder = millis();
    } else if(currentClockPins < 0) {
    SetClockLength(60);
    currentClockPins = 59;
    }

    if(startTest) {
    // SetClockLength(50);
    SetClockTime(10);
    startTest = false;
    }*/

  loopCount++;

  if(loopCount == 100) {
    Serial.write('v');
    Serial.write('\0');
  }
//  } else if(loopCount < 100) {
//    int mod = loopCount % 5;
//    if(mod == 0) {
//      UpdateInnerStripColor(color1);
//      UpdateStripColor(color2);
//    } else if(mod == 1) {
//      UpdateInnerStripColor(color2);
//      UpdateStripColor(color3);
//    } else if(mod == 2) {
//      UpdateInnerStripColor(color3);
//      UpdateStripColor(color4);
//    } else if(mod == 3) {
//      UpdateInnerStripColor(color4);
//      UpdateStripColor(color5);
//    } else if(mod == 4) {
//      UpdateInnerStripColor(color5);
//      UpdateStripColor(color1);
//    }
//  }

  testingVoteBlink++;

  if(testingVoteBlink > 40) {
    VotingBlink();
    votingBlinkLoops++;
  }
}

// function to visually draw the lights of a train
void drawTrain(int i, uint32_t head, uint32_t tail) {
  outerStrip.setPixelColor(((i + (NUMBER_OF_PINS_OUTER - 1)) % NUMBER_OF_PINS_OUTER), 50, 50, 50);
  outerStrip.setPixelColor((i) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 1) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 2) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 3) % NUMBER_OF_PINS_OUTER, head);
}

void drawTrainInner(int i, uint32_t head) {
  innerStrip.setPixelColor(((i + (NUMBER_OF_PINS_INNER - 1)) % NUMBER_OF_PINS_INNER), 50, 50, 50);
  innerStrip.setPixelColor((i) % NUMBER_OF_PINS_INNER, head);
  innerStrip.setPixelColor((i+1) % NUMBER_OF_PINS_INNER, head);
}

// function to spawn a new train
void SpawnTrain(byte sender, byte receiver, byte trainID) {
  //  SendDebugMessage("Spawning a train at Player " + sender + " and sending to Player " + receiver + "with trainID = " + trainID + "\n");
  uint32_t fromColor = GetColor(sender);
  uint32_t toColor = GetColor(receiver);

  int playerNumber = (ToInt(sender) - 1) * TRAINS_PER_PLAYER;
  int startPosition = (trainStation + (nodesPerSector * (ToInt(sender) - 1)));
  drawTrain(startPosition, fromColor, toColor);
//  outerStrip.show();

//  SendDebugMessage(sender);
//  SendDebugMessage(receiver);
//  SendDebugMessage(trainID);

  // update arrays for the new train that was just spawned
  for (int i = 0; i < TRAINS_PER_PLAYER; i++) {
    if (trainsID[playerNumber + i] == 0) {
      location[playerNumber + i] = startPosition;
      headColors[playerNumber + i] = fromColor;
      tailColors[playerNumber + i] = toColor;
      trainsStopped[playerNumber + i] = 0;
      trainsID[playerNumber + i] = ToInt(trainID);
      SendDebugMessage("TRAIN SUCCESSFULLY SPAWNED");
      break;
    }
  }
}

// function to destroy a train
void DestroyTrain(byte trainID) {
  //SendDebugMessage("Destroying a train with trainID = " + trainID + "\n");
  int trainIndex = GetTrainIndex(ToInt(trainID));
  /*Adafruit_NeoPixel strip;

  if (answeredTrains[trainID]) {
    strip = innerStrip;
  }
  else {
    strip = outerStrip;
  }*/

  if(!answeredTrains[trainIndex]) {   
    // erase where the train previously was before being stopped
    ResetPin(location[trainIndex]);
    ResetPin(location[trainIndex] + 1);
    ResetPin(location[trainIndex] + 2);
    ResetPin(location[trainIndex] + 3);
//    outerStrip.show();
  } else {
    ResetPinInner(location[trainIndex]);
    ResetPinInner(location[trainIndex] + 1);
//    innerStrip.show();
  }

  location[trainIndex] = 0;
  headColors[trainIndex] = 0;
  tailColors[trainIndex] = 0;
  trainsStopped[trainIndex] = 0;
  trainsID[trainIndex] = 0;
  answeredTrains[trainIndex] = 0;
}

// function to pause a train
void PauseTrain(byte hijacker, byte trainID) {
  int hijackerNumber = ToInt(hijacker) - 1;
  int trainIndex = GetTrainIndex(ToInt(trainID));

  if(!answeredTrains[trainIndex]) {
    // erase where the train previously was before being stopped
    SendDebugMessage("PAUSE TRAIN: RESETING OUTER STRIP");
    ResetPin(location[trainIndex]);
    ResetPin(location[trainIndex] + 1);
    ResetPin(location[trainIndex] + 2);
    ResetPin(location[trainIndex] + 3);
    location[trainIndex] = (trainStation + (nodesPerSector * hijackerNumber));
  } else {
    ResetPinInner(location[trainIndex]);
    ResetPinInner(location[trainIndex] + 1);
    location[trainIndex] = (innerStation + (nodesPerInnerSector * hijackerNumber));
  }

  // set location to the station of the player who hijacked it
  trainsStopped[trainIndex] = 1;
}

// function to release a paused train
void ReleaseTrain(byte trainID) {
  //SendDebugMessage("Released a train paused with trainID = " + trainID + "\n");
  int trainIndex = GetTrainIndex(ToInt(trainID));

  // need to set location to the players station
  trainsStopped[trainIndex] = 0;
}

// function to get the color of a player id
uint32_t GetColor(byte id) {
  if (id == (byte)1) {
    return color1;
  } else if (id == (byte)2) {
    return color2;
  } else if (id == (byte)3) {
    return color3;
  } else if (id == (byte)4) {
    return color4;
  } else if (id == (byte)5) {
    return color5;
  } else {
    SendDebugMessage("ELSE IN GET COLOR");
    SendDebugMessage(id);
    return outerStrip.Color(0, 0, 0);
  }
}

// function to change the speed that lights are delayed by
void SetSpeed(byte newSpeed) {
//  if (ToInt(questionStrip) == 0) {
//    SendDebugMessage("question strip was 0");
//  } else if (ToInt(questionStrip) == 1) {
//    SendDebugMessage("question strip was 1");
//  }
  //set to time between station
  delayTime = ToInt(newSpeed);
}

// function to update the outer strip to a single color
void UpdateStripColor(uint32_t color) {
  for (int i = 0; i < NUMBER_OF_PINS_OUTER; i++) {
    outerStrip.setPixelColor(i, color);
  }

//  outerStrip.show();
}

// function to update the inner strip to a single color
void UpdateInnerStripColor(uint32_t color) {
  for (int i = 0; i < NUMBER_OF_PINS_INNER; i++) {
    innerStrip.setPixelColor(i, color);
  }

//  innerStrip.show();
}

// function to reset a pin to the base color of the outer strip
void ResetPin(int pin) {
  outerStrip.setPixelColor(pin, outerStrip.Color(50, 50, 50));
}

// function to reset a pin to the base color of the inner strip
void ResetPinInner(int pin) {
  innerStrip.setPixelColor(pin, innerStrip.Color(50, 50, 50));
}

// function to reset the game for a new game
void ResetGame() {
  UpdateStripColor(outerStrip.Color(50, 50, 50)); // update strip to the base color
  UpdateInnerStripColor(innerStrip.Color(50, 50, 50));

  // reset all arrays that store train information
  for (int i = 0; i < 5 * TRAINS_PER_PLAYER; i++) {
    trainsID[i] = 0;
    trainsStopped[i] = 0;
    answeredTrains[i] = 0;
    location[i] = 0;
    headColors[i] = 0;
    tailColors[i] = 0;
  }

  for(int i=0; i<10; i++) {
    playerVotes[i] = blankColor;
  }
}

// function to answer a train and move it to the answered strip
void AnswerTrain(byte trainID) {
  int trainIndex = GetTrainIndex(ToInt(trainID));
  int playerNumber = 0;

  for (int i = 0; i < 5; i++) {
    if (location[trainIndex] < ((i + 1) * (NUMBER_OF_PINS_OUTER / 5))) {
      playerNumber = i;
      break;
    }
  }

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex]);
  ResetPin(location[trainIndex] + 1);
  ResetPin(location[trainIndex] + 2);
  ResetPin(location[trainIndex] + 3);
//  outerStrip.show();

  location[trainIndex] = (nodesPerInnerSector * (playerNumber));
  trainsStopped[trainIndex] = 0;
  answeredTrains[trainIndex] = 1;
}

// function to set who a player is voting for
void Vote(byte senderID, byte vote1, byte vote2) {
  int playerNumber = ToInt(senderID) - 1;
  int voteIndex = playerNumber * 2;

  playerVotes[voteIndex] = GetColor(vote1);
  playerVotes[voteIndex + 1] = GetColor(vote2);
}

void GenerateVoteColors(int player, uint32_t v1, uint32_t v2) {
//  uint32_t c1 = allColors[vote1];
//  uint32_t c2 = allColors[vote2];
//
//  if(vote1 == 1) { c1 = color1; }
//  else if(vote1 == 2) { c1 = color2; }
//  else if(vote1 == 3) { c1 = color3; }
//  else if(vote1 == 4) { c1 = color4; }
//  else if(vote1 == 5) { c1 = color5; }
//
//  if(vote2 == 1) { c2 = color1; }
//  else if(vote2 == 2) { c2 = color2; }
//  else if(vote2 == 3) { c2 = color3; }
//  else if(vote2 == 4) { c2 = color4; }
//  else if(vote2 == 5) { c2 = color5; }

  outerStrip.setPixelColor(110 + (player * 2), v1);
  outerStrip.setPixelColor(111 + (player * 2), v2);
}

// helper function to turn a trainID into its corresponding index value
int GetTrainIndex(int value) {
  for (int i = 0; i < 5 * TRAINS_PER_PLAYER; i++) {
    if (trainsID[i] == value) {
      return i;
    }
  }

  return -1;
}

// helper function to convert a char into an int
int ToInt(byte c) {
  if (c < 30) {
    return c;
  }
  int ret = c - (byte)'0';
  return ret;
}

// function to adjust the current length of the clock
void SetClockLength(int len, Adafruit_NeoPixel strip) {
  currentClockPins = len;

  uint32_t funColor = allColors[tester];
  tester = (tester + 1) % 5;

  for (int i = 0; i < 60; i++) {
    if (i < currentClockPins) {
      strip.setPixelColor(i, funColor);
    } else {
      strip.setPixelColor(i, baseColor);
    }
  }

//  strip.show();
}

// function to adjust how long the clock will take to go through
void SetClockTime(int time_) {
  clockTick = time_;
}

// function to allow the track to blink when it is close to voting time
void VotingBlink() {
  if(votingBlinkLoops > 5 && votingBlink) {
    votingBlinkLoops = 0;
    votingBlink = false;
    UpdateStripColor(baseColor);
    UpdateInnerStripColor(baseColor);
  } else if(votingBlinkLoops > 5 && !votingBlink) {
    votingBlinkLoops = 0;
    votingBlink = true;
    UpdateStripColor(voteBlinkColor);
    UpdateInnerStripColor(voteBlinkColor);
  }
}


// DEBUGGING IN UNITY
void SendDebugMessage(char message[]) {
  Serial.print('s');
  Serial.print(message);
  Serial.write('\n');
  Serial.write('\0');
}

void SendDebugMessage(char message) {
  Serial.print('s');
  Serial.print(message);
  Serial.write('\n');
  Serial.write('\0');
}

void SendDebugMessage(byte message) {
  Serial.print('s');
  Serial.print(message);
  Serial.write('\n');
  Serial.write('\0');
}

void SendDebugMessage(const char message[]) {
  Serial.print('s');
  Serial.print(message);
  Serial.write('\n');
  Serial.write('\0');
}

void SendDebugMessage(char* message, int len) {
  Serial.write('s');
  Serial.write(message, len);
  Serial.write('\n');
  Serial.write('\0');
}
