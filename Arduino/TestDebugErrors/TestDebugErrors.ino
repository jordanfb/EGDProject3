#include <Key.h>
#include <Keypad.h>
#include "Adafruit_Thermal.h"


#include "Arduino.h"
//#include "Keypad.h"

const char ARDUINO_ID = 1; // 1 to 6
const String playerIDToStringName[] = {"", "Red", "Blue", "Green", "Purple", "Yellow"}; // no player 0, because that's the null terminating character so we don't send that if we can avoid it


const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {5, 6, 7, 8}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {2, 3, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// letter tracking
unsigned long lastKeypressTime = 0;
unsigned long timeBetweenLetters = 750;
char lastKeyPress = '\0';
int numPresses = 0;

String keypadInput = "";


// printer stuff
#include "SoftwareSerial.h"
#define TX_PIN 10 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 9 // Arduino receive   GREEN WIRE   labeled TX on printer

SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor


// game state data!
bool isInitializedForGame = false; // reset to false when new game is called, set true when we get the codewords etc.
bool isTownsperson = true;
int numCodewords = 0;
char codewords[5][65]; // max 5 codewords/keywords and max 64 character long strings
//char wordBuffer[65]; // max 64 character long strings for everything. Probably way too much
int createdTrains = 0;
int allowedTrains = 1; // for now just allowed trains is 1, we may add a way to change that in the messaging system we'll see.
bool creatingLeftTrain = true;

// train data!
char trainFrom = 0;
char trainTo = 0;
int leftWordLength = 0;
char leftWord[65]; // max 64 character long strings for everything. Probably way too much
int rightWordLength = 0;
char rightWord[65]; // max 64 character long strings for everything. Probably way too much
int answerWordLength = 0;
char questionAnswer[65]; // max 64 character long strings for everything. Probably way too much

String debugStringTest = "";
//int incomingCommandLength = 0;
//char incomingCommand[256]]; // used for storing command stuff I guess

void setup(){
  Serial.begin(9600);
  while (!Serial); // wait for it to be ready
  while(Serial.available()) {Serial.read(); } // clear the serial for now?

  // NOTE: SOME PRINTERS NEED 9600 BAUD instead of 19200, check test page. (From the test printer we got it needed 19200, but we'll see what happens for the rest of them)
  mySerial.begin(19200);  // Initialize SoftwareSerial
  //Serial1.begin(19200); // Use this instead if using hardware serial
  printer.begin();        // Init printer (same regardless of serial type)
}

void SendDebugMessage(char message[]) {
  Serial.write('s');
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

void SendDebugMessage(byte message) {
  Serial.write('s');
  Serial.write(message);
  Serial.write('\n');
  Serial.write('\0');
}

void HandleIncomingSerial() {
//  while (Serial.available() > 0) {
//    // skip over characters
//    if (Serial.read() == '<') {
//      // we've found the start of a command
////      SendDebugMessage("Found start of command");
//      break;
//    }
//  }
//  while (Serial.available() == 0) {
//    // wait for serial stuff to appear!
////    return; // ignore these characters
//  }
  char messageType = Serial.read();
  if (messageType == 0) {
    SendDebugMessage('E');
  } else {
    SendDebugMessage(messageType);
  }
  
  switch(messageType) {
    case 'a':
      SendDebugMessage("Found who are you comand");
      // Who are you?
      // send back our player ID with command I am ____
      Serial.read(); // should be null character
      
      Serial.write('r');
      Serial.write(ARDUINO_ID);
      Serial.write(0);
      SendDebugMessage("Got who are you");
    break;
    default:
      debugStringTest += messageType;
      SendDebugMessage("Got message I couldn't read");
      SendDebugMessage(messageType);
      char c = 1;
      while (c != 0) {
        // keep reading until the message is over
        c = Serial.read();
        debugStringTest += c;
        SendDebugMessage(c);
        if (c == 63) {
          // literally a question mark
          SendDebugMessage("Literally a question mark");
        }
      }
      SendDebugMessage("Recieved:");
      debugStringTest.toCharArray(leftWord, debugStringTest.length());
      SendDebugMessage(leftWord, debugStringTest.length());
      SendDebugMessage("Done sending bytes I couldn't read");
      break; // we could error but for now just ignore it
  }
}










void loop(){
  if (Serial.available()) {
    HandleIncomingSerial();
    // then echo the string back
  }
}
