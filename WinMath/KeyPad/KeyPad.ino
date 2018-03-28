
#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
const int gate =13;
const char password[8] = {'7','8','9','6','9','1','1','2'};
char hexaKeys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad
unsigned long t = 0;
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

void setup(){
  Serial.begin(9600);
  pinMode(gate,OUTPUT);
  digitalWrite(gate,HIGH);
  Serial.println("Start");
}
  
void loop(){
  char customKey = customKeypad.getKey();  
  if (customKey == '*'){
    Serial.println("OK");
    t = millis();
    char code[8];
    int i = 0;
    do{
      customKey = customKeypad.getKey(); 
      if(customKey){
        code [i] = customKey;
        Serial.print(customKey);
        i++;
      }
      if(customKey=='*'){
        Serial.println("exit");
        break;
      }
      if(i>7){
        Serial.println("exit");
        break;
      }
    }while(millis()-t <5000);
    if(strncmp(password,code,8)==0){
        Serial.println("Access");
        digitalWrite(gate,LOW);
        delay(5000);
        digitalWrite(gate,HIGH);
    }else{
        Serial.println("Denied");
        digitalWrite(gate,HIGH);
    }
  }
}
