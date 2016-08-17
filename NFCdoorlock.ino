/*/The following Code is the Arduino NFC FREA-K v2.0 Code Developed by Chirag Parmar, Swaroop and Projjal Gupta
 * Feel free to use this code.
 * Copyright© Next Tech Labs, SRM
 */
#include <RFID.h>
#include <EEPROM.h>
#include <SPI.h>

int ledgr = 3;  //green led indicator for access grant
int ledred = 4; //red led indicator for access revoked
int lock = 2; //pinout for lock
int buttonState;
int panic = 6;

const int tagSize = 4;

struct NFCDATA {
  int id;
  byte data[tagSize];
};

RFID rfid(10,5);    // rfid variable with SS/SDA pin at DIO 10 and RST pin at DIO 5

NFCDATA tag;
int rid = 0;
int address = 0;         //stores the tag addresses value
byte serNum[tagSize];     //stores the read rfid data
byte master[tagSize] = {0x1D,0x07F,0xE8,0x0D5}; // master card data(do not store in the eeprom someone might read from it) 

void setup(){  
  Serial.begin(9600); // initialize serial communication
  SPI.begin(); // initialize SPI communication for RFID
  
  pinMode(ledgr, OUTPUT);
  pinMode(ledred, OUTPUT);
  pinMode(lock, OUTPUT);
  pinMode(panic,INPUT_PULLUP);
  
  address = getcurAddress();
  rfid.init(); // initialize the RFID  
  Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
  Serial.println("+                    FREA-K (version 2.0)                   +");
  Serial.println("+           Please choose from the following:               +");
  Serial.println("+           - Scan your card to open the lock               +");
  Serial.println("+           - Use Master Card for advanced options          +");
  Serial.println("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

}
 
void loop(){
  sreadTag();
  if(compareTag(master,tag.data)) {
    Serial.println("Add a tag or swipe once more to remove a tag");
    delay(1000);
    sreadTag();
    if(compareTag(master,tag.data)) {
      Serial.println("Remove a Tag");
      rid = inputID();
      removeTag(rid);
    }
    else {
      addTag();
    }
  }
  else if(!checkTag(tag)) {
    //switch on green light
    digitalWrite(ledgr, HIGH);
    Serial.println("Access Granted");
    door();
    digitalWrite(ledgr, LOW);
  }
  else if(checkTag(tag)) {
    digitalWrite(ledred, HIGH);
    Serial.println("Access Denied");
    digitalWrite(lock, LOW);
    delay(1000);
    digitalWrite(ledred, LOW);
  }
}

void printalltags() {
  NFCDATA printtag;
  int addr = 0;
  while(addr < address) {
    EEPROM.get(addr, printtag);
    Serial.print("Id no. = ");
    Serial.println(printtag.id);
    Serial.print("Tag Data : ");
    for(int i=0;i<tagSize;i++) {
      Serial.print(printtag.data[i] , HEX);
      Serial.print(" ");
    }
    Serial.println("");
    Serial.println("");
    addr = addr + sizeof(printtag);
  }
}

/* will read the tags from the rfid sensor and store it into the variable 'data'
 *  it will create a loop of itself until a card is read i.e. when this function 
 *  is called a card must be kept on the sensor(mandatory).
 *  it gives us advantage over the previous method because u dont have to loop the 
 *  whole of 'loop' function over and over again.
 */
void sreadTag() {
  Serial.println("Place the tag to read");
start:
  if(rfid.isCard()) {
    rfid.readCardSerial();
    for(int i=0;i<tagSize;i++) {
      tag.data[i] = rfid.serNum[i]; 
    }
  Serial.println("Card Read");
  }
  else {
    buttonState = digitalRead(panic);
    if(buttonState == LOW) {
      door();
    }
    goto start;
  }
}


/* compares the data of two tags that are passed to the function returns 1
 *  if the tags match otherwise returns 0
 */
boolean compareTag(byte arr1[],byte arr2[]) {
  int m,n,flag;
  flag = 0;
  m = sizeof(arr1)/sizeof(byte);
  n = sizeof(arr2)/sizeof(byte);
  if(m==n) {
    for(int i=0;i<tagSize;i++) {
      if(arr1[i] != arr2[i]) {
        flag = 1;
        return false;
        break;
      }
    }
    if(flag == 0) return true;
  }
  else return false;
}

/* adds a tag with automatic id generation i.e. you dont have to specify the id no. 
 *  for each tag you are adding it will give you the id no. after adding the tag.(incomplete)
 *  write the code so that it prints the id no. on the serial monitor each id refers to 
 *  7 bytes of data. consider the initial two bytes of data reserved for the address 
 */
void addTag() {
  int id = 0;
  if(checkTag(tag)) {
    id = inputID();
    tag.id = id;
    EEPROM.put(address, tag);
    address = address + sizeof(tag);
    EEPROM.write(address,0xFF);
    Serial.println("Tag data written");
  }
  else Serial.println("Tag already exists");
}

/*checks if the passed tag is present in the directory or not. Returns 1 if not previously added
 * else reurns 0 if tag already exists this is the case of you want to add a tag. otherwise you can 
 * use this function to check for authorization to the lab
 */
boolean checkTag(NFCDATA tag) {
  int checkAddr = 0;
  NFCDATA temptag;
  while(checkAddr < address) {
    EEPROM.get(checkAddr,temptag);
    if(compareTag(temptag.data,tag.data)) {
      return false;
      break;
    }
    checkAddr = checkAddr + sizeof(temptag);
  }
  return true;
}


/*removes a tag specified by id and leaves the memory location empty
 * the id no. of the empty id is stored so that if next time of someone tries to 
 * add a tag the empty location gets filled up first ( you have to write the code for that in the loop function)
 */
void removeTag(int id) {
  int lastAddr = getcurAddress();
  int addr = 0;
  NFCDATA lastTag,empty;
  lastAddr = lastAddr - sizeof(lastTag);
  addr = checkID(id);
  EEPROM.get(lastAddr,lastTag);
  if(lastTag.id == id) {
    EEPROM.put(addr,empty); 
  }
  else {
    EEPROM.put(addr,lastTag);
    EEPROM.put(lastAddr,empty);
  }
}


int inputID() {
  int id;
  Serial.println("Enter the ID : ");
  start:
  if(Serial.available()) {
    id = Serial.parseInt();
    if(id > 0) return id;
    else goto start;
  }
  else goto start;
}

int getcurAddress() {
  int addr;
  byte value;
  for(addr = 0;addr < EEPROM.length();addr++) {
    value = EEPROM.read(addr);
    if(value == 0xFF) {
      return addr;
      break;
    }
  }
  return 0;
}

int checkID(int id) {
  int checkAddr = 0;
  NFCDATA temptag;
  while(checkAddr < address) {
    EEPROM.get(checkAddr,temptag);
    if(temptag.id == id) {
      return checkAddr;
      break;
    }
    checkAddr = checkAddr + sizeof(temptag);
  }
  Serial.println("ID not found");
}


void door()
{
  digitalWrite(lock, HIGH);
  delay(3000);
  digitalWrite(lock, LOW);
 }
