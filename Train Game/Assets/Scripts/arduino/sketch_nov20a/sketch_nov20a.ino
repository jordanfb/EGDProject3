int clientNumber = 2;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  while(!Serial);

  int test = 2;
  switch(test){
    case 1:
      Serial.println("1");
      break;
    case 2:
      Serial.println("2");
      break;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.peek() == -1){
    return;
  }
  int input = Serial.read();
  Serial.println(input);
  switch (input) {
    case 97:
      {
      Serial.println("case 1");
      byte message[3] = {'r', clientNumber, 0};
      
      Serial.write(message, 3);
      break; 
      }
      
    case 110:
      {
      Serial.println("case 2");
      char message2[14] = {'n', 1, 2, 's', 'i', 'm','o', 'n', '\n', 'n','a','t','e', '\0'};
      Serial.write(message2, 14);
      break; 
        
      }
      
    default:
      // statements
      break;
    
  }
  delay(10);
}
