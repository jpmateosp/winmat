
#include <SPI.h>
#include <SD.h>
#include "RF24.h"

byte addresses[][6] = {"WinD","WinR","Master"};

bool radioNumber = 1;
RF24 radio(7,8);
bool role = 0;
int button = 0;

struct dataStruct{
  byte ID[4];
  bool access;
}myData;
File myFile;

void setup() {

  Serial.begin(9600);
  pinMode(2,INPUT);         //Boton
  pinMode(3,OUTPUT);        //LED acceso
  pinMode(4,OUTPUT);        //SPI SD
  digitalWrite(3,LOW);
  digitalWrite(4,HIGH);     //Desactivar SPI SD

  if (!SD.begin(4)) {
    Serial.println("SD:ER");
  }else
    Serial.println("SD:OK");
    
  radio.begin();
  radio.setPALevel(RF24_PA_LOW); 
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  radio.startListening();
  Serial.println("RF:OK");
}

void loop() {

    if(button>4){
       button=0;
       myData.access = 1;
       radio.stopListening();                                    // First, stop listening so we can talk.
       if (!radio.write( &myData, sizeof(myData) )){
          Serial.println(F("Re:ER;"));
       }
       radio.startListening();                                    // Now, continue listening
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
          delay(2000);
      }else{
                                                                  // Grab the response, compare, and send to debugging spew
          radio.read( &myData, sizeof(myData) );
          unsigned long time = micros();                  
          // Spew it
          Serial.print(F("Re:OK "));
          myData.access = 0;
          delay(2000);
      }


     }
     
     else if( radio.available()){
      button = 0;                                                     // Variable for the received timestamp
      while (radio.available()) {                          // While there is data ready
        radio.read( &myData, sizeof(myData) );             // Get the payload
      }
      radio.stopListening();                               // First, stop listening so we can talk  
      digitalWrite(7,HIGH);
      digitalWrite(4,LOW);
      delay(1);
      if(myData.ID[0]>0&&myData.ID[1]>0){
    
          char aux[10];
          char buf[10];
          for ( uint8_t i = 0; i < 4; i++) {  //
              itoa (myData.ID[i],aux+(2*i),16);
          }
          aux[8]=';';
          aux[9]=0;
          myFile = SD.open("Win/Access.txt");
          if (myFile) {
              Serial.println("OP:OK;");
              myData.access = 0;
              while (myFile.available()) {
                myFile.read(buf,9);
                if(strstr(buf,aux) > 0)
                 {
                    myData.access = 1;
                    break;     
                 }
              }
            } else {
              Serial.println("OP:ER;");
            }
          
          myFile.close();
                                          // Increment the float value
          
          digitalWrite(4,HIGH);
          digitalWrite(7,LOW);
      }else{
          myData.access = 0;
      }
          radio.write( &myData, sizeof(myData) );              // Send the final one back.      
          radio.startListening();                              // Now, resume listening so we catch the next packets.     
          Serial.print(F("ID"));
          Serial.print(F(":"));
          for ( uint8_t i = 0; i < 4; i++) {  //
                        Serial.print(myData.ID[i], HEX);
                      }
          Serial.println(";");
      
      
      
     }else
      button = button+digitalRead(2); 


} // Loop
