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



void setup(){
  Serial.begin(9600);

  // NOTE: SOME PRINTERS NEED 9600 BAUD instead of 19200, check test page. (From the test printer we got it needed 19200, but we'll see what happens for the rest of them)
  mySerial.begin(19200);  // Initialize SoftwareSerial
  //Serial1.begin(19200); // Use this instead if using hardware serial
  printer.begin();        // Init printer (same regardless of serial type)
}

void charPressedRemap(char c) {
  if (c == '1') {
    return; // ignore those...
  }
  unsigned long currtime = millis();
  if (lastKeyPress != c) {
    // add the key!
    if (lastKeyPress != '\0') {
      keypadInput += getLetterFromMap(lastKeyPress, numPresses);
    }
    lastKeyPress = c;
    numPresses = 1;
  } else {
    if (currtime - lastKeypressTime < timeBetweenLetters) {
      // increment the letter
      numPresses++;
    } else {
      keypadInput += getLetterFromMap(lastKeyPress, numPresses);
      lastKeyPress = c;
      numPresses = 1;
    }
  }
  lastKeypressTime = currtime;
}

void ResetCreatedWordsAndAnswer() {
  leftWordLength = 0;
  leftWord[0] = '\0';
  rightWordLength = 0;
  rightWord[0] = '\0';
  answerWordLength = 0;
  questionAnswer[0] = '\0';
  creatingLeftTrain = true;
}

char getLetterFromMap(char keypadChar, int numPresses) {
//  Serial.println(keypadChar);
  numPresses -= 1; // so we can use mod
  switch(keypadChar) {
    case '2':
      numPresses %= 3;
      return char(numPresses + 97 + 0);
    case '3':
      Serial.println("Got to 3");
      numPresses %= 3;
      return char(numPresses + 97 + 3);
    case '4':
      numPresses %= 3;
      return char(numPresses + 97 + 6);
    case '5':
      numPresses %= 3;
      return char(numPresses + 97 + 9);
    case '6':
//      Serial.println("Got to 6");
      numPresses %= 3;
      return char(numPresses + 97 + 12);
    case '7':
      numPresses %= 4;
      return char(numPresses + 97 + 15);
    case '8':
      numPresses %= 3;
      return char(numPresses + 97 + 19);
    case '9':
      numPresses %= 4;
      return char(numPresses + 97 + 22);
    case '0':
//      Serial.println("got to space");
      return ' ';
    default:
      return '\0';
      break;
  }
}

void NewGame() {
  // reset variables
  lastKeypressTime = 0;
  lastKeyPress = '\0';
  numPresses = 0;
  keypadInput = "";
  isInitializedForGame = false;
  createdTrains = 0;
  creatingLeftTrain = true;

  ResetCreatedWordsAndAnswer();

  // also ensure we have a null character at the end.
  leftWord[64] = '\0';
  rightWord[64] = '\0';
  questionAnswer[64] = '\0';

  // print out help messages to players!
}

void SendDebugMessage(char message[]) {
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

void HandleIncomingSerial() {
  char messageType = Serial.read();

  switch(messageType) {
    case 'a':
      // Who are you?
      // send back our player ID with command I am ____
      while (!Serial.available());
      Serial.read(); // should be null character
      
      Serial.write('r');
      Serial.write(ARDUINO_ID);
      Serial.write('\0');
      SendDebugMessage("Got who are you");
    break;
    case 'g':
      // display train don't answer
      // print it out and start a countdown to doing other things likely
      ReadInTrainMessages(true);
      while (!Serial.available());
      Serial.read();
      DisplayTrain(true);
      break;
    case 'h':
      // display train you need to answer
      // print it out and require that you choose one or the other thing!
      ReadInTrainMessages(false);
      while (!Serial.available());
      Serial.read();
      DisplayTrain(false);
      break;
    case 'k':
      // new game! Reset everything!
      while (!Serial.available());
      Serial.read(); // should be null character
      NewGame();
      break;
    case 'l':
      // send keywords
      // store that we're a townsperson and also what our keywords are and also tell the player
      while (!Serial.available());
      numCodewords = Serial.read();
      numCodewords = min(numCodewords, 5);
      // then readin  the codewords!
      SendDebugMessage("entered read in codewords");
      ReadInCodewords(numCodewords);
      SendDebugMessage("done reading code words");
      while (!Serial.available());
      if(Serial.read() ==0){
        SendDebugMessage("ended with null character");
      } // should be null character
      isTownsperson = true;
      isInitializedForGame = true;
      DisplayCodewords();
      SendDebugMessage("Got Townsfolk keywords");
      break;
    case 'm':
      // send code words
      // store that we're a spy and also what our code words are and also tell the player
      while (!Serial.available());
      numCodewords = Serial.read();
      numCodewords = min(numCodewords, 5);
      // then read in the codewords!
      SendDebugMessage("entered read in codewords");
      ReadInCodewords(numCodewords);
      SendDebugMessage("done reading code words");
      while (!Serial.available());
      if(Serial.read() ==0){
        SendDebugMessage("ended with null character");
      } // should be null character
      
      isTownsperson = false;
      isInitializedForGame = true;
      Serial.write('s');
      for (int i = 0; i < numCodewords; i++) {
        int c = 0;
        while (codewords[i][c] != '\0') {
          Serial.write(codewords[i][c]);
          c++;
        }
      }
      Serial.write('\n');
      Serial.write('\0');
      DisplayCodewords();
      SendDebugMessage("Got spy keywords");
      break;
    default:
      SendDebugMessage("Got message I couldn't read");
      SendDebugMessage(&messageType, 1);
//      char c = 1;
//      while (c != '\0') {
//        // keep reading until the message is over
//        c = Serial.read();
//        SendDebugMessage(&c, 1);
//      }
//      SendDebugMessage("Done sending bytes I couldn't read");
      break; // we could error but for now just ignore it
  }
}

int ReadStringNewlineEnded(char buff[]) {
  // buffer has to be 65 characters long, word outputs may only be 64 characters long.
  
  int currWordLength = 0;
  // read in this codeword
  // codewords end in newlines
  while (!Serial.available());
  char c = Serial.read();
  while (c != '\n' && c != '\0' && currWordLength < 64) {
      // add it to the current word
      buff[currWordLength] = c;
      currWordLength++;
      while (!Serial.available());
      c = Serial.read();
    }
    // if it's a newline, then we're set! For now just assume it's a newline I guess?
//    while (c != '\n' && c != '\0' ) {
//      // if it exited due to word length then read until we get the newline, this should never occur since we won't have too long codewords but just in case.
//      c = Serial.read();
//    }
    buff[currWordLength] = '\0'; // end it!
    return currWordLength;
}

void DisplayTrain(bool includeAnswer) {
  printer.println(F("New message:"));
  printer.print(F("FROM: "));
  printer.println(playerIDToStringName[trainFrom]);  
  printer.print(F("TO: "));
  printer.println(playerIDToStringName[trainTo]);
  
  // display the contents of the message
  printer.println(leftWord);
  printer.print(F(" OR "));
  printer.println(rightWord);

  if (includeAnswer) {
    if (answerWordLength > 0) {
      printer.print(F("Answer: "));
      printer.println(questionAnswer);
    }
    else {
      printer.println(F("Not yet answered."));
    }
  }
}

void ReadInTrainMessages(bool includeAnswer) {
  while (!Serial.available());
  trainFrom = Serial.read();
  while (!Serial.available());
  trainTo = Serial.read();
  leftWordLength = ReadStringNewlineEnded(leftWord);
  leftWordLength = ReadStringNewlineEnded(rightWord);
  if (includeAnswer) {
    answerWordLength = ReadStringNewlineEnded(questionAnswer);
  } else {
    answerWordLength = 0;
    questionAnswer[0] = '\0';
  }
}



void DisplayCodewords() {
  // loop through them all to display the codewords.
  printer.inverseOn();
  if (isTownsperson) {
    printer.println(F("You are a TOWNSPERSON"));
    printer.inverseOff();
//    printer.println(F("The spies DO NOT KNOW these keywords! Ask other players questions about them to determine if they are in the know and vote them out if they are a spy!"));
  } else {
    printer.println(F("You are a SPY"));
    printer.inverseOff();
//    printer.println(F("Blend in among the townspeople and send these codewords to the other spy in order to reduce the time that the townsfolk have to vote you out!"));
  }
  printer.justify('C');
  for (int i =0; i < numCodewords; i++) {
    // print out the codewords!
    printer.println(codewords[i]);
  }
  printer.justify('L');
}

void ReadInCodewords(int num) {
  // read in num codewords and store them in the list
  for (int i =0; i < num; i++) {
    ReadStringNewlineEnded(codewords[i]);
//    int currWordLength = 0;
//    // read in this codeword
//    // codewords end in newlines
//    char c = Serial.read();
//    while (c != '\n' && currWordLength < 64) {
//      // add it to the current word
//      codewords[i][currWordLength] = c;
//      currWordLength++;
//      c = Serial.read();
//    }
//    // if it's a newline, then we're set! For now just assume it's a newline I guess?
//    while (c != '\n') {
//      // if it exited due to word length then read until we get the newline, this should never occur since we won't have too long codewords but just in case.
//      c = Serial.read();
//    }
//    codewords[i][currWordLength] = '\0'; // end it!
  }
  for (int i = num; i < 5; i++) {
    codewords[i][0] = '\0'; // destroy the other codewords.
  }
}

void loop(){
  if (isInitializedForGame) {
    // if we're playing the game!
    char key = keypad.getKey();

    if (millis() - lastKeypressTime >= timeBetweenLetters) {
      // increment the letter
      if (lastKeyPress != '\0') {
        keypadInput += getLetterFromMap(lastKeyPress, numPresses);
        lastKeyPress = '\0';
        numPresses = 1;
      }
    }
    
    if (key != NO_KEY) {
      Serial.println(key);
      if (key == '*') {
        if (lastKeyPress != '\0') {
          lastKeyPress = '\0';
          numPresses = 0;
        }
        else {
          keypadInput = keypadInput.substring(0, keypadInput.length()-1);
          lastKeyPress = '\0';
          numPresses = 0;
        }
      } else if (key == '#'){
        if (keypadInput.length() > 0 || lastKeyPress != '\0') {
          if (lastKeyPress != '\0') {
            // then add it on
            keypadInput += getLetterFromMap(lastKeyPress, numPresses);
            lastKeyPress = '\0';
            numPresses = 1;
          }
          Serial.println(keypadInput);
          keypadInput = "";
        }
      } else {
//        keypadInput += key;
        charPressedRemap(key);
      }
    }
  }
  if (Serial.available()) {
    HandleIncomingSerial();
    // then echo the string back
//    Serial.println("Recieved: " + Serial.readString());
  }
}
