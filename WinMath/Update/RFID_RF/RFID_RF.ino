#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include "RF24.h"

#define DOOR  2
  
////RFID Initialization///

const char RST_PIN = 9;     // Configurable pin
const char SS_PIN = 10;     // Configurable pin

unsigned int successRead;    // Variable integer to keep if we have Successful Read from Reader
unsigned char storedCard[4];          // Stores an ID read from EEPROM
unsigned char readCard[4];            // Stores scanned ID read from RFID Module
unsigned char buffer[18];

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

   const unsigned char pass[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };

////RF24 Initialization///

byte addresses[][6] = {"WinD","WinR","Master"};
bool radioNumber = 0;
RF24 radio(7,8);

struct dataStruct{
  unsigned char ID[4];
  bool access;
  bool ack;
}myData;


void setup() {
  pinMode(DOOR,OUTPUT);
  digitalWrite(DOOR,HIGH);
  
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  
  RFID_init();
  RF24_init();
}

void loop() {
  if(getID()){
    if(read_block()){
       if(check_pass(buffer)){
          Serial.println("Attempt");
          if(RF_Request()== 0){
              if(myData.access){
                  Open_door();
              }else{
                  digitalWrite(DOOR,HIGH);
                  Serial.println("No Access");
              }
          }else{
            Serial.println("Error");
          }
       }else{
          Serial.println("No match");
       }
    }else{
        Serial.println("Try again");
    }
    digitalWrite(DOOR,HIGH);
    RFID_reset();
  }else if(radio.available()){
    if(RF24_GetReq()){
      Serial.println("Message good");
      if(myData.access){
        RF24_Relpy();
        Open_door();
      }else{
        Serial.println("No Access");
      }
    }else{
      Serial.println("Message no good");
    }
  }else{
    RFID_reset();
  }
}



void RFID_init(){
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  //Set Antenna Gain to Max it will increase reading distance
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
  }
}

bool getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");


  return 1;
}
bool read_block(){
  MFRC522::StatusCode status;
  byte size = sizeof(buffer);
  int blockAddr      = 4;
  int trailerBlock   = 7;
  
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return 0;
  }
      // Read data from the block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return 0;
  }
  Serial.println("Block Read");
  return 1;


}
bool check_pass(unsigned char* buf){
  for(int i = 0; i<12; i++){
    if (pass[i] == buf[i]);
     else{return 0;}
    }
    return 1;
}

void RFID_reset(){
    // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}


void RF24_init(){
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
  radio.startListening();
}

int RF_Request(){
  strncpy(myData.ID, readCard, 4);
  myData.ack = 0;
  radio.stopListening();                                    // First, stop listening so we can talk.
  radio.write( &myData, sizeof(myData));
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
      Serial.println(F("Timeout"));
      return 2;
  }else{
                                                              // Grab the response, compare, and send to debugging spew
      radio.read( &myData, sizeof(myData) );
      if(myData.ack){
        return 0;
      }else
        return 3;
  }    
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

void RF24_Relpy(){
  myData.ack = 1;
  radio.write( &myData, sizeof(myData) );              // Send the final one back.      
  radio.startListening();                              // Now, resume listening so we catch the next packets.     
}

void Open_door(){
  digitalWrite(DOOR,LOW);
  Serial.println("Access");
  delay(3000);
  digitalWrite(DOOR,HIGH);
  }
