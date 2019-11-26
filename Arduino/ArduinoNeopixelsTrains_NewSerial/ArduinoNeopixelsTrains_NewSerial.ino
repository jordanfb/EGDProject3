#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#define PIN 6
#define INNER_PIN 5
#define NUMBER_OF_PINS_OUTER 300
#define NUMBER_OF_PINS_INNER 60
#define TRAINS_PER_PLAYER 2

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel outerStrip = Adafruit_NeoPixel(NUMBER_OF_PINS_OUTER, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel innerStrip = Adafruit_NeoPixel(NUMBER_OF_PINS_INNER, INNER_PIN, NEO_GRB + NEO_KHZ800);

// variables
int delayTime = 60;
int nodesPerSector = NUMBER_OF_PINS_OUTER / 5;
int nodesPerInnerSector = NUMBER_OF_PINS_INNER / 5;
int trainStation = 0;
int innerStation = 0;
unsigned long timeHolder = 0;
int clockTick = 750;
int currentClockPins = 59;

// Player colors, color number corresponds to player number, feel free to change these color values here
uint32_t color1 = outerStrip.Color(255, 0, 0);
uint32_t color2 = outerStrip.Color(0, 0, 255);
uint32_t color3 = outerStrip.Color(0, 255, 0);
uint32_t color4 = outerStrip.Color(255, 0, 255);
uint32_t color5 = outerStrip.Color(255, 255, 0);
uint32_t baseColor = outerStrip.Color(25, 25, 25);
uint32_t allColors[] = {color1, color2, color3, color4, color5};
int tester = 1;

// arrays for each info for a players trains
int playerVotes[5 * TRAINS_PER_PLAYER];
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

  //UpdateStripColor(baseColor, outerStrip);
  //UpdateStripColor(color5, innerStrip);

  trainStation = (nodesPerSector / 2) - 2;
  innerStation = (nodesPerInnerSector / 2);

  timeHolder = millis();
}

void loop() {
  // loop through all possible trains and run the functionality they should have at that time
  for (int i = 0; i < (5 * TRAINS_PER_PLAYER); i++) {
    //    location[1] += 1;
    //      drawTrain(location[1], outerStrip.Color(50, 0, 0), outerStrip.Color(50, 50, 0));
    //      outerStrip.show();
    if (trainsID[i] != 0 && !trainsStopped[i]) { // if there is a spawned train and its not stopped
      //      SendDebugMessage("Train moving at i =");
      //      SendDebugMessage((byte)i);
      location[i] += 1; // move the train forward one pin
      if (location[i] >= NUMBER_OF_PINS_OUTER) {
        location[i] %= NUMBER_OF_PINS_OUTER;
      }

      drawTrain(location[i], headColors[i], tailColors[i]); // draw the train on the strip
      outerStrip.show();
    } else if (trainsStopped[i]) { // if train is stopped
      //      SendDebugMessage("Train stopped at i =");
      //      SendDebugMessage((byte)i);
        drawTrain(location[i], headColors[i], tailColors[i]); // draw the train at the stopped station
        outerStrip.show();
      //      outerStrip.show();
      //      delay(delayTime);
    } /* else if (answeredTrains[i]) {
        ...
        drawTrainInner(location[i], headColors[i]);
    }*/
  }

  //outerStrip.show();
  delay(delayTime);

  if (Serial.peek() != -1) {
    char currentRead = Serial.read();
    messageReady = false;

    if (currentRead == '\0' || currentRead == '\n') {
      currentByte = 0;
      messageReady = true;
      SendDebugMessage("MESSAGE READY");
    } else {
      buffer_[currentByte] = currentRead;
      currentByte++;
    }
  }

  if (messageReady) {

    switch (buffer_[0]) {
      case (byte)'a': // "Who are you?" case
        SendDebugMessage("WHO ARE YOU?:\n");
        Serial.write('r');
        Serial.write((byte)6);
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
        SetSpeed(buffer_[1], buffer_[2]);
        break;
      case 'j': // send player votes in
        SendDebugMessage("CASTING VOTES:\n");
        Vote(buffer_[1], buffer_[2], buffer_[3]);
        break;
      case 'k': // new game
        SendDebugMessage("RESET GAME:\n");
        ResetGame();
        break;
      default:
        //      Serial.println(buffer_[0]);
        SendDebugMessage("WTF");
        SendDebugMessage(buffer_[0]);
        SendDebugMessage((char)buffer_[0]);
        break;
    }

    messageReady = false;
    //buffer_.clear();
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
}

// function to visually draw the lights of a train
void drawTrain(int i, uint32_t head, uint32_t tail) {
  //outerStrip.setPixelColor((i+(NUMBER_OF_PINS_OUTER - 1))%NUMBER_OF_PINS_OUTER, 20, 20, 20);
  //SendDebugMessage((byte)i);
  outerStrip.setPixelColor(((i + (NUMBER_OF_PINS_OUTER - 1)) % NUMBER_OF_PINS_OUTER), 0, 0, 0);
  outerStrip.setPixelColor((i) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 1) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 2) % NUMBER_OF_PINS_OUTER, tail);
  outerStrip.setPixelColor((i + 3) % NUMBER_OF_PINS_OUTER, head);
}

void drawTrainInner(int i, uint32_t head) {
  innerStrip.setPixelColor((i) % NUMBER_OF_PINS_INNER, head);
}

// function to spawn a new train
void SpawnTrain(byte sender, byte receiver, byte trainID) {
  //  SendDebugMessage("Spawning a train at Player " + sender + " and sending to Player " + receiver + "with trainID = " + trainID + "\n");
  uint32_t fromColor = GetColor(sender);
  uint32_t toColor = GetColor(receiver);

  int playerNumber = (ToInt(sender) - 1) * TRAINS_PER_PLAYER;
  int startPosition = (trainStation + (nodesPerSector * (ToInt(sender) - 1)));
  drawTrain(startPosition, fromColor, toColor);
  outerStrip.show();

  //SendDebugMessage("I'M HERE");

  SendDebugMessage(sender);
  SendDebugMessage(receiver);
  SendDebugMessage(trainID);

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

  //SendDebugMessage("COULD NOT SPAWN TRAIN!\n");
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

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex], outerStrip);
  ResetPin(location[trainIndex] + 1, outerStrip);
  ResetPin(location[trainIndex] + 2, outerStrip);
  ResetPin(location[trainIndex] + 3, outerStrip);
  outerStrip.show();

  location[trainIndex] = 0;
  headColors[trainIndex] = 0;
  tailColors[trainIndex] = 0;
  trainsStopped[trainIndex] = 0;
  trainsID[trainIndex] = 0;
  //answeredTrains[trainIndex] = 0;
}

// function to pause a train
void PauseTrain(byte hijacker, byte trainID) {
  //SendDebugMessage("Paused a train at Player " + hijacker + " that had trainID = " + trainID + "\n");
  int hijackerNumber = ToInt(hijacker) - 1;
  int trainIndex = GetTrainIndex(ToInt(trainID));
 // Adafruit_NeoPixel strip;

  /*if (answeredTrains[trainID]) {
    strip = innerStrip;
  }
  else {
    strip = outerStrip;
  }*/

SendDebugMessage("IN PAUSE TRAIN: ");
SendDebugMessage((byte)trainIndex);

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex], outerStrip);
  ResetPin(location[trainIndex] + 1, outerStrip);
  ResetPin(location[trainIndex] + 2, outerStrip);
  ResetPin(location[trainIndex] + 3, outerStrip);
  //outerStrip.show();

  // set location to the station of the player who hijacked it
  location[trainIndex] = (trainStation + (nodesPerSector * hijackerNumber));
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
    return outerStrip.Color(50, 0, 0);
  }
}

// function to change the speed that lights are delayed by
void SetSpeed(byte newSpeed, byte questionStrip) {
  if (ToInt(questionStrip) == 0) {
    SendDebugMessage("question strip was 0");
  } else if (ToInt(questionStrip) == 1) {
    SendDebugMessage("question strip was 1");
  }
  //set to time between station
  delayTime = ToInt(newSpeed);
}

// function to update the strip to a single color
void UpdateStripColor(uint32_t color, Adafruit_NeoPixel strip) {
  for (int i = 0; i < NUMBER_OF_PINS_OUTER; i++) {
    strip.setPixelColor(i, color);
  }

  strip.show();
}

// function to reset a pin to the base color of the strip
void ResetPin(int pin, Adafruit_NeoPixel strip) {
  strip.setPixelColor(pin, baseColor);
}

// function to reset the game for a new game
void ResetGame() {
  UpdateStripColor(outerStrip.Color(0, 0, 0), outerStrip); // update strip to the base color
  //UpdateStripColor(baseColor, innerStrip);

  // reset all arrays that store train information
  for (int i = 0; i < 5 * TRAINS_PER_PLAYER; i++) {
    trainsID[i] = 0;
    trainsStopped[i] = 0;
    answeredTrains[i] = 0;
    location[i] = 0;
    headColors[i] = 0;
    tailColors[i] = 0;
  }
}

// function to answer a train and move it to the answered strip
void AnswerTrain(byte trainID) {
  //SendDebugMessage("Destroying a train with trainID = " + trainID + "\n");
  int trainIndex = GetTrainIndex(ToInt(trainID));
  int playerNumber = 0;

  for (int i = 0; i < 5; i++) {
    if (location[trainIndex] < ((i + 1) * (NUMBER_OF_PINS_OUTER / 5))) {
      playerNumber = i;
      break;
    }
  }

  // erase where the train previously was before being stopped
  ResetPin(location[trainIndex], outerStrip);
  ResetPin(location[trainIndex] + 1, outerStrip);
  ResetPin(location[trainIndex] + 2, outerStrip);
  ResetPin(location[trainIndex] + 3, outerStrip);
  outerStrip.show();

  location[trainIndex] = (innerStation + (nodesPerInnerSector * (playerNumber)));
  trainsStopped[trainIndex] = 0;
  answeredTrains[trainIndex] = 1;

  // DstroyTrain(trianID)
  // maybe have new array to hold inner values
  // OR have a bool array for if the train was answered and then populate information and add a check for inner loop and put on that strip
}

// function to set who a player is voting for
void Vote(byte senderID, byte vote1, byte vote2) {
  int playerNumber = ToInt(senderID) - 1;
  int voteIndex = playerNumber * 2;

  playerVotes[voteIndex] = ToInt(vote1);
  playerVotes[voteIndex + 1] = ToInt(vote2);
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

  strip.show();
}

// function to adjust how long the clock will take to go through
void SetClockTime(int time_) {
  clockTick = time_;
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
