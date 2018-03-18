
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>        // RC522 Module uses SPI protocol
#include <MFRC522.h>  // Library for Mifare RC522 Devices
#include "RF24.h"

constexpr uint8_t RST_PIN = 9;     // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 10;     // Configurable, see typical pin layout above

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader
byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

   const byte pass[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
        0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
    };

byte addresses[][6] = {"Door","Rec","Master"};
bool radioNumber = 0;
RF24 radio(7,8);
bool role = 1;

struct dataStruct{
  byte ID[4];
  bool access;
}myData;

void setup() {

  //Protocol Configuration
  Serial.begin(9600);  // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  //Set Antenna Gain to Max it will increase reading distance
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
  }
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);

}

void loop () {
  do {
    successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0
     
     }while (!successRead);   //the program will not go further while you are not getting a successful read
  if(successRead){        
            MFRC522::StatusCode status;
            byte buffer[18];
            byte size = sizeof(buffer);
            byte blockAddr      = 4;
            byte trailerBlock   = 7;
            
            status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
            if (status != MFRC522::STATUS_OK) {
                Serial.print(F("PCD_Authenticate() failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
                return;
            }
                // Read data from the block
            status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
            if (status != MFRC522::STATUS_OK) {
                Serial.print(F("MIFARE_Read() failed: "));
                Serial.println(mfrc522.GetStatusCodeName(status));
            }
            if(check_pass(buffer)){
              strncpy(myData.ID, readCard, 4);
              radio.stopListening();                                    // First, stop listening so we can talk.
              Serial.println(F("REQ:"));
               if (!radio.write( &myData, sizeof(myData) )){
                 Serial.println(F("failed"));
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
                  Serial.println(F("Failed, response timed out."));
              }else{
                                                                          // Grab the response, compare, and send to debugging spew
                  radio.read( &myData, sizeof(myData) );
                  unsigned long time = micros();
                  
                  // Spew it
                  Serial.print(F("Sent "));
                  Serial.print(F("ACCESS"));
                  Serial.print(myData.access);
                  Serial.print(F("ID "));
                  for ( uint8_t i = 0; i < 4; i++) {  //
                    Serial.print(myData.ID[i], HEX);
                  }
              }
          
              // Try again 1s later
              delay(1000);
  
              }else
              Serial.println("UNKNOWN");
            //dump_byte_array(buffer, 16); Serial.println();
            
            Serial.println();
            
              // Halt PICC
            mfrc522.PICC_HaltA();
            // Stop encryption on PCD
            mfrc522.PCD_StopCrypto1();
  }else{
            mfrc522.PICC_HaltA();
            // Stop encryption on PCD
            mfrc522.PCD_StopCrypto1();
    }
  
}


uint8_t getID() {
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

bool check_pass(byte* buf){
  for(int i = 0; i<12; i++){
    if (pass[i] == buf[i]);
     else{return 0;}
    }
    return 1;
  }

void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
