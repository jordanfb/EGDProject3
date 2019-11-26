#include <Key.h>
#include <Keypad.h>
#include "Adafruit_Thermal.h"


#include "Arduino.h"
//#include "Keypad.h"

#define STOP_PIN 11
#define VOTE_PINS 12 // starts from this pin and goes up 4. i.e. 12, 13, 14, 15 // shouldn't use pin 13 though because that has the blink LED there though, so we should figure this out...
#define TIME_BETWEEN_PRESSES 4000
const char ARDUINO_ID = 5; // 1 to 6
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

// debouncing:
unsigned long stopTrainDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


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
bool answeringTrain = false;

// train data!
char trainID = 1;
char trainFrom = 0;
char trainTo = 0;
int leftWordLength = 0;
char leftWord[65]; // max 64 character long strings for everything. Probably way too much
int rightWordLength = 0;
char rightWord[65]; // max 64 character long strings for everything. Probably way too much
int answerWordLength = 0;
char questionAnswer[65]; // max 64 character long strings for everything. Probably way too much

int stopTrainAllowedTime = 0;

void setup(){
  Serial.begin(9600);

  // NOTE: SOME PRINTERS NEED 9600 BAUD instead of 19200, check test page. (From the test printer we got it needed 19200, but we'll see what happens for the rest of them)
  mySerial.begin(19200);  // Initialize SoftwareSerial
  //Serial1.begin(19200); // Use this instead if using hardware serial
  printer.begin();        // Init printer (same regardless of serial type)

  // begin button pins!
  pinMode(STOP_PIN, INPUT_PULLUP); // the other end of it should be connected to ground!
  pinMode(VOTE_PINS, INPUT_PULLUP); // the other end of it should be connected to ground!
  pinMode(VOTE_PINS, INPUT_PULLUP + 1); // the other end of it should be connected to ground!
  pinMode(VOTE_PINS, INPUT_PULLUP + 2); // the other end of it should be connected to ground!
  pinMode(VOTE_PINS, INPUT_PULLUP + 3); // the other end of it should be connected to ground!
}

void AddCharacterToTrain(char c) {
  // if it's safe to add the character then I do so, otherwise we print an error I guess
  if (creatingLeftTrain) {
    if (leftWordLength < 64) {
      // then allow it
      leftWord[leftWordLength] = c;
      leftWordLength++;
      leftWord[leftWordLength] = '\0'; // make sure it's null terminated
    } else {
      printer.println(F("Your first word is the maximum 64 characters long!"));
    }
  } else {
    // right train!
    if (rightWordLength < 64) {
      // then allow it
      rightWord[rightWordLength] = c;
      rightWordLength++;
      rightWord[rightWordLength] = '\0'; // make sure it's null terminated
    } else {
      printer.println(F("Your second word is the maximum 64 characters long!"));
    }
  }
}

void charPressedRemap(char c) {
  if (c == '1') {
    return; // ignore those...
  }
  unsigned long currtime = millis();
  if (lastKeyPress != c) {
    // add the key!
    if (lastKeyPress != '\0') {
//      keypadInput += getLetterFromMap(lastKeyPress, numPresses);
      AddCharacterToTrain(getLetterFromMap(lastKeyPress, numPresses));
    }
    lastKeyPress = c;
    numPresses = 1;
  } else {
    if (currtime - lastKeypressTime < timeBetweenLetters) {
      // increment the letter
      numPresses++;
    } else {
//      keypadInput += getLetterFromMap(lastKeyPress, numPresses);
      AddCharacterToTrain(getLetterFromMap(lastKeyPress, numPresses));
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
//      Serial.println("Got to 3");
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
  stopTrainAllowedTime = 0; // allowed to stop a train!
  answeringTrain = false;

  ResetCreatedWordsAndAnswer();

  // also ensure we have a null character at the end.
  leftWord[64] = '\0';
  rightWord[64] = '\0';
  questionAnswer[64] = '\0';

  // print out help messages to players! FIX
//  printer.println("Welcome to the incredibe game of Three Dimensional Chess! In this game you will be playing against your opponent, Captain Picard of the USS Enterprise");
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

void SendVote(char voteFor) {
  Serial.write('q');
  Serial.write(ARDUINO_ID);
  Serial.write(voteFor);
  Serial.write('\0');

  printer.print("Voted against the ");
  printer.print(playerIDToStringName[voteFor]);
  printer.println(" player");
}

void CreateTrain(char destination) {
  // sender, reciever, left\n, right\n
  Serial.write('n');
  Serial.write(ARDUINO_ID);
  Serial.write(destination);
  for (int i =0; i < leftWordLength; i++) {
    Serial.write(leftWord[i]);
  }
  Serial.write('\n');
  for (int i =0; i < rightWordLength; i++) {
    Serial.write(rightWord[i]);
  }
  Serial.write('\n');
  Serial.write('\0');
}

void AnswerQuestion(bool chooseRight) {
  Serial.write('p');
  Serial.write(ARDUINO_ID);
  Serial.write(trainID);
  if (chooseRight) {
    Serial.write((byte)2);
    Serial.write('\0');
    SendDebugMessage("Answered question with right");
  } else {
    Serial.write((byte)1);
    Serial.write('\0');
    SendDebugMessage("Answered question with left");
  }
}

void SendStopTrain() {
  Serial.write('o');
  Serial.write(ARDUINO_ID);
  Serial.write('\0');
}

void HandleButtonPresses() {
  // deal with voting and stop train buttons!
  unsigned long t = millis(); //  && (t - stopTrainDebounceTime > debounceTime)
  if (t > stopTrainAllowedTime) {
    // if we eventually get lighting up the stop train button to work put it here! FIX Also, if we get this working make sure to disable the pin on new game
    if (digitalRead(STOP_PIN) == LOW) {
      // stop the train!
      SendStopTrain();
      SendDebugMessage("STOP TRAIN PRESSED");
      // prevent players from pressing this again for some time
      stopTrainAllowedTime = t + TIME_BETWEEN_PRESSES;
    }
  } else {
    if (digitalRead(STOP_PIN) == LOW) {
      SendDebugMessage("Stop train pressed but it's not time to press it");
    }
  }

  // now check for voting pins! Can always vote!
  // these are pulled high so if they're low they're pressed
  char vote = 0;
  if (digitalRead(VOTE_PINS) == LOW) {
    // vote lowest number
    vote = 1;
  } else if (digitalRead(VOTE_PINS + 1) == LOW) {
    // vote lowest number
    vote = 2;
  } else if (digitalRead(VOTE_PINS + 2) == LOW) {
    // vote lowest number
    vote = 3;
  } else if (digitalRead(VOTE_PINS + 3) == LOW) {
    // vote lowest number
    vote = 4;
  }
  if (vote >= ARDUINO_ID) {
    // then increment it by 1 since you can't vote for yourself!
    vote++;
  }
  // don't send vote at the moment because we don't have buttons hooked up there
//  if (vote > 0) {
//    // then you voted this frame! Send the vote!
//    SendVote(vote);
//  }
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
//      SendDebugMessage("Got who are you");
    break;
    case 'g':
      // display train don't answer
      // print it out and start a countdown to doing other things likely
      ReadInTrainMessages(true);
      while (!Serial.available());
      Serial.read();
      printer.println(F("Intercepted a train:"));
      DisplayTrain(true);
      ResetCreatedWordsAndAnswer(); // so that players are able to start typing again when the train is gone
      break;
    case 'h':
      // display train you need to answer
      // print it out and require that you choose one or the other thing!
      while (!Serial.available());
      trainID = Serial.read(); // read the train ID
      ReadInTrainMessages(false);
      while (!Serial.available());
      Serial.read();
      printer.println(F("Stopped a train to you:"));
      DisplayTrain(false);
      ResetCreatedWordsAndAnswer(); // so that players are able to start typing again when the train is gone
      answeringTrain = true;
      printer.println(F("Press 4 to choose the left option. Press 6 to choose the right option."));
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
//      SendDebugMessage("entered read in codewords");
      ReadInCodewords(numCodewords);
//      SendDebugMessage("done reading code words");
      while (!Serial.available());
      if(Serial.read() ==0){
//        SendDebugMessage("ended with null character");
      } // should be null character
      isTownsperson = true;
      isInitializedForGame = true;
      DisplayCodewords();
//      SendDebugMessage("Got Townsfolk keywords");
      break;
    case 'm':
      // send code words
      // store that we're a spy and also what our code words are and also tell the player
      while (!Serial.available());
      numCodewords = Serial.read();
      numCodewords = min(numCodewords, 5);
      // then read in the codewords!
//      SendDebugMessage("entered read in codewords");
      ReadInCodewords(numCodewords);
//      SendDebugMessage("done reading code words");
      while (!Serial.available());
      if(Serial.read() ==0){
//        SendDebugMessage("ended with null character");
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
//      SendDebugMessage("Got spy keywords");
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
  printer.println(F(" OR "));
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
  if (trainFrom == ARDUINO_ID) {
    createdTrains--;
    if (createdTrains < 0) {
      createdTrains = 0;
    }
    printer.println("Stopped one of your trains and thus are able to make another one!");
  }
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
    if (answeringTrain) {
      // then make them answer the traiiin!
      // if they press 4 or 6 then choose left or right!
      char key = keypad.getKey();
      if (key != NO_KEY) {
        // then they chose a key, is it a good key?
        if (key == '4') {
          // then answer left!
          AnswerQuestion(false);
          answeringTrain = false;
        } else if (key == '6') {
          // answer right!
          AnswerQuestion(true);
          answeringTrain = false;
        }
      }
    } else {
      // if we're playing the game!
      char key = keypad.getKey();

      if (millis() - lastKeypressTime >= timeBetweenLetters) {
        // increment the letter
        if (lastKeyPress != '\0') {
//          keypadInput += getLetterFromMap(lastKeyPress, numPresses);
          AddCharacterToTrain(getLetterFromMap(lastKeyPress, numPresses));
          lastKeyPress = '\0';
          numPresses = 1;
        }
      }
    
      if (key != NO_KEY) {
//        Serial.println(key); // don't do that that will fuck up the game code
        if (key == '1')
        {
          if (createdTrains < allowedTrains) {
            if (leftWordLength > 0 && rightWordLength > 0) {
              // then DEBUG SEND THE TRAIN I GUESS?
              // Improve this when we have the correct keypads
              printer.println(F("DEBUG: Choose who you're sending it to, press their character 1, 2, 3, 4, or 5"));
              while (keypad.getKey() == '1') {
                // keep reading until we release the 1 key
              }
              while (true) {
                // choose who to send it to
                key = keypad.getKey();
                if (key != NO_KEY) {
                  // is it a valid choice?
                  if (key == '1' || key == '2' || key == '3' || key == '4' || key == '5') {
                    // send it to that person!
                      printer.print("Sent it to player ");
                    printer.println(key);
                    CreateTrain(key - '0');
                    createdTrains++;
                    ResetCreatedWordsAndAnswer(); // so that players are able to start typing again when the train is gone
                    SendDebugMessage("CREATED TRAIN PLEASE");
                    break;
                  }
                }
              }
            } else if (rightWordLength > 0){
              printer.println("Left word has length 0");
            }
            else if (leftWordLength > 0){
              printer.println("Right word has length 0");
            } else {
              printer.println("Can't send, you haven't entered any message!");
            }
          }
          else {
            // not allowed to create any more trains
            printer.println(F("Not allowed to create any more trains!"));
          }
        }
        else if (key == '*') {
          if (lastKeyPress != '\0') {
            lastKeyPress = '\0';
            numPresses = 0;
          }
          else {
            if (creatingLeftTrain) {
              if (leftWordLength > 0) {
                leftWordLength--;
                leftWord[leftWordLength] = '\0';
              }
            } else {
              if (rightWordLength > 0) {
                rightWordLength--;
                rightWord[rightWordLength] = '\0';
              }
            }
//            keypadInput = keypadInput.substring(0, keypadInput.length()-1);
            lastKeyPress = '\0';
            numPresses = 0;
          }
        } else if (key == '#'){
          // toggle between left and right word
          if (creatingLeftTrain) {
            printer.print("Your first option is currently: '");
            for(int i = 0; i < leftWordLength; i++) {
              printer.print(leftWord[i]);
            }
            printer.println("'"); // close quotes
          } else {
            printer.print("Your second option is currently: '");
            for(int i = 0; i < rightWordLength; i++) {
              printer.print(rightWord[i]);
            }
            printer.println("'"); // close quotes
          }
          creatingLeftTrain = !creatingLeftTrain;
//          if (keypadInput.length() > 0 || lastKeyPress != '\0') {
//            if (lastKeyPress != '\0') {
//              // then add it on
//              keypadInput += getLetterFromMap(lastKeyPress, numPresses);
//              lastKeyPress = '\0';
//              numPresses = 1;
//            }
////            Serial.println(keypadInput); // don't do that that will fuck up the game code, instead, send a train! (or toggle which you're editing! FIX
//            keypadInput = "";
//          }
        } else {
//        keypadInput += key;
          charPressedRemap(key);
        }
      }
      // if we're playing the game then
      HandleButtonPresses();
    }
  }
  
  if (Serial.available()) {
    HandleIncomingSerial();
    // then echo the string back
//    Serial.println("Recieved: " + Serial.readString());
  }
}
