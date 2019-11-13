#include "Arduino.h"
#include "Keypad.h"

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


String input = "";

void setup(){
  Serial.begin(9600);
}

void charPressedRemap(char c) {
  if (c == '1') {
    return; // ignore those...
  }
  unsigned long currtime = millis();
  if (lastKeyPress != c) {
    // add the key!
    if (lastKeyPress != '\0') {
      input += getLetterFromMap(lastKeyPress, numPresses);
    }
    lastKeyPress = c;
    numPresses = 1;
  } else {
    if (currtime - lastKeypressTime < timeBetweenLetters) {
      // increment the letter
      numPresses++;
    } else {
      input += getLetterFromMap(lastKeyPress, numPresses);
      lastKeyPress = c;
      numPresses = 1;
    }
  }
  lastKeypressTime = currtime;
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

void loop(){
  char key = keypad.getKey();

  if (millis() - lastKeypressTime >= timeBetweenLetters) {
    // increment the letter
    if (lastKeyPress != '\0') {
      input += getLetterFromMap(lastKeyPress, numPresses);
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
        input = input.substring(0, input.length()-1);
        lastKeyPress = '\0';
        numPresses = 0;
      }
    } else if (key == '#'){
      if (input.length() > 0 || lastKeyPress != '\0') {
        if (lastKeyPress != '\0') {
          // then add it on
          input += getLetterFromMap(lastKeyPress, numPresses);
          lastKeyPress = '\0';
          numPresses = 1;
        }
        Serial.println(input);
        input = "";
      }
    } else {
//      input += key;
      charPressedRemap(key);
    }
  }
  if (Serial.peek() != -1) {
    // then echo the string back
    Serial.println("Recieved: " + Serial.readString());
  }
}
