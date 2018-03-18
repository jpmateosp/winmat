#include <SPI.h>
#include <SD.h>
#include "RF24.h"

////RF24 Initialization///

byte addresses[][6] = {"WinD","WinR","Master"};
bool radioNumber = 1;
RF24 radio(7,8);

struct dataStruct{
  unsigned char ID[4];
  bool access;
  bool ack;
}myData;

////Button Initialization///

int button = 0;

////SD Initialization///
File myFile;


void setup() {
  Serial.begin(9600);
  pinMode(2,INPUT);         //Boton
  pinMode(3,OUTPUT);        //LED acceso
  pinMode(4,OUTPUT);        //SPI SD
  pinMode(5,OUTPUT);        //SD ERROR
  digitalWrite(3,LOW);
  do{
    digitalWrite(5,HIGH);
  }while(!SD.begin(4));
  digitalWrite(5,LOW);
  Serial.println("SD:OK");
  RF24_init();
}

void loop() {
  if( radio.available()){
    if(RF24_GetReq()){
      Serial.println("Message good");
      SD_check();
      if(myData.access){
        RF24_Relpy();
        Serial.println("Access sent");
      }else{
        Serial.println("No Access");
      }
    }else{
      Serial.println("Message no good");
    }
  }else if(digitalRead(2)){
      Serial.println("Activation triggered");
      radio.stopListening();                                    // First, stop listening so we can talk.
      myData.access = 1;
      myData.ack = 0;
      radio.write( &myData, sizeof(myData));
      radio.startListening();
      unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
      boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
       while ( ! radio.available() ){                             // While nothing is received
          if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
              timeout = true;
              break;
          }      
       }                
      if ( timeout ){                                             // Describe the results
          Serial.println(F("TO"));
          
      }else{
                                                                  // Grab the response, compare, and send to debugging spew
          radio.read( &myData, sizeof(myData) );
          if(myData.ack){
            Serial.print(F("Door Open"));
          }else{
            Serial.print(F("No response"));
          }
      }
      delay(2000);
      
  }else{
      //CHECK if SD works.
  }

}


void RF24_init(){
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); 
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  radio.startListening();
}

bool RF24_GetReq(){
  while (radio.available()) {                          // While there is data ready
        radio.read( &myData, sizeof(myData) );             // Get the payload
      }
      radio.stopListening();
      Serial.println("GOT");
      if(myData.ack == 0){
        return 1;
      }
      else{
        return 0;
        }
}

bool RF24_Relpy(){
  myData.ack = 1;
  radio.write( &myData, sizeof(myData) );              // Send the final one back.      
  radio.startListening();                              // Now, resume listening so we catch the next packets.     
  Serial.print(F("ID"));
  Serial.print(F(":"));
  for ( uint8_t i = 0; i < 4; i++) {  //
                Serial.print(myData.ID[i], HEX);
              }
  Serial.println(";");
}

void SD_check(){
  char aux[10];
  char buf[10];
  
  digitalWrite(7,HIGH);
  digitalWrite(4,LOW);
  delay(1);

  for ( int i = 0; i < 4; i++) {  //
      itoa (myData.ID[i],aux+(2*i),16);
  }
  aux[8]=';';
  aux[9]=0;
  myFile = SD.open("Win/Access.txt");
  if (myFile) {
      Serial.println("OP:OK;");
      while (myFile.available()) {
        myFile.read(buf,9);
        if(strstr(buf,aux) > 0){
            myData.access = 1;
            break;     
         }else
            myData.access = 0;
      }
    }else {
      myData.access = 0;
      Serial.println("OP:ER;");
      digitalWrite(5,HIGH);
    }
  myFile.close();
  digitalWrite(4,HIGH);
  digitalWrite(7,LOW);
}

