#include <EtherCard.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "pitches.h"

// notes in the melody:
int melody[] = {
  NOTE_ALARM, 0, NOTE_ALARM, 0, NOTE_ALARM, 0, NOTE_ALARM, 0
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 16, 4, 16, 4, 16, 4, 16
};


#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24

int L3G4200D_Address = 105; //I2C address of the L3G4200D
StaticJsonBuffer<50> jsonBuffer;
int x;
int y;
int z;
static uint32_t timer;
static uint32_t timer2;

const int LED_INTERNET = 9;
const int LED_POWER = 5;
const int SPEAKER = 3;
int stash_size;
// ethernet interface mac address, must be unique on the LAN
byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

const char website[] PROGMEM = "0w3aj2.messaging.internetofthings.ibmcloud.com";
const char website2[] PROGMEM = "hello-node-noisy-mongoose.eu-gb.mybluemix.net";
static byte session;
byte Ethernet::buffer[400];
Stash stash;

void setupGyro(){
  Wire.begin();
  Serial.begin(57600);

  Serial.println("starting up L3G4200D");
  setupL3G4200D(2000); // Configure L3G4200  - 250, 500 or 2000 deg/sec

  delay(1500); //wait for the sensor to be ready 
}

void getGyroValues(){

  byte xMSB = readRegister(L3G4200D_Address, 0x29);
  byte xLSB = readRegister(L3G4200D_Address, 0x28);
  x = ((xMSB << 8) | xLSB);

  byte yMSB = readRegister(L3G4200D_Address, 0x2B);
  byte yLSB = readRegister(L3G4200D_Address, 0x2A);
  y = ((yMSB << 8) | yLSB);

  byte zMSB = readRegister(L3G4200D_Address, 0x2D);
  byte zLSB = readRegister(L3G4200D_Address, 0x2C);
  z = ((zMSB << 8) | zLSB);
}

int setupL3G4200D(int scale){
  //From  Jim Lindblom of Sparkfun's code

  // Enable x, y, z and turn off power down:
  writeRegister(L3G4200D_Address, CTRL_REG1, 0b00001111);

  // If you'd like to adjust/use the HPF, you can edit the line below to configure CTRL_REG2:
  writeRegister(L3G4200D_Address, CTRL_REG2, 0b00000000);

  // Configure CTRL_REG3 to generate data ready interrupt on INT2
  // No interrupts used on INT1, if you'd like to configure INT1
  // or INT2 otherwise, consult the datasheet:
  writeRegister(L3G4200D_Address, CTRL_REG3, 0b00001000);

  // CTRL_REG4 controls the full-scale range, among other things:

  if(scale == 250){
    writeRegister(L3G4200D_Address, CTRL_REG4, 0b00000000);
  }else if(scale == 500){
    writeRegister(L3G4200D_Address, CTRL_REG4, 0b00010000);
  }else{
    writeRegister(L3G4200D_Address, CTRL_REG4, 0b00110000);
  }

  // CTRL_REG5 controls high-pass filtering of outputs, use it
  // if you'd like:
  writeRegister(L3G4200D_Address, CTRL_REG5, 0b00000000);
}

void writeRegister(int deviceAddress, byte address, byte val) {
    Wire.beginTransmission(deviceAddress); // start transmission to device 
    Wire.write(address);       // send register address
    Wire.write(val);         // send value to write
    Wire.endTransmission();     // end transmission
}

int readRegister(int deviceAddress, byte address){

    int v;
    Wire.beginTransmission(deviceAddress);
    Wire.write(address); // register to read
    Wire.endTransmission();

    Wire.requestFrom(deviceAddress, 1); // read a byte

    while(!Wire.available()) {
        // waiting
    }

    v = Wire.read();
    return v;
}

void setup () {
  setupGyro();
  Serial.begin(57600);
  
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
  Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
  Serial.println(F("DHCP failed"));
  
  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
  
  if (!ether.dnsLookup(website))
  Serial.println(F("DNS failed"));
  
  
  //getReq();
  getReq(); 
}

void loop () {
  getGyroValues();  // This will update x, y, and z with new values
  ether.packetLoop(ether.packetReceive());
  const char* reply = ether.tcpReply(session);
    if (reply != 0) {
    Serial.println(reply);
  }
  
  if (millis() > timer) {
    timer = millis() + 2000;
    getReq();
  }
    char buffer [2];

  //delay (500);
  if (millis() > timer2) {
    timer2 = millis() + 1000;
    //ether.browseUrl(PSTR("/check"),buffer,website2,my_callback);
  }
  
  //checkEarthquake();
  /*const char* reply2 = ether.tcpReply(session);
    if (reply2 != 0) {
    Serial.print("Earthquake status");
    Serial.println(reply2);
  }*/
  
}

static void my_callback (byte status, word off, word len) {
  delay(50);
  Serial.println(">>>");
  Ethernet::buffer[off + 300] = 0;
  Serial.print((const char*) Ethernet::buffer + off);
  Serial.println("...");
}



#include "pitches.h"

void earhquake_alarm( int PIN_NUMBER) {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(PIN_NUMBER, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(PIN_NUMBER);
  }
}

void alert() {
  while (true) {
     digitalWrite(LED_INTERNET, HIGH);   // turn the LED on (HIGH is the voltage level)
     delay(100);                       // wait for a second
     digitalWrite(LED_INTERNET, LOW);    // turn the LED off by making the voltage LOW
     delay(100); 
  }
}

void powerLedOn() {
  digitalWrite(LED_POWER, HIGH);   
}

void internetLedBlink() {
     digitalWrite(LED_INTERNET, HIGH);   // turn the LED on (HIGH is the voltage level)
     delay(200);                       // wait for a second
     digitalWrite(LED_INTERNET, LOW);    // turn the LED off by making the voltage LOW
     delay(200); 
}



static void checkEarthquake(){ 
  byte sd = stash.create();
  stash.save();
  
  stash_size = stash.size();

  Stash::prepare(PSTR("GET http://hello-node-noisy-mongoose.eu-gb.mybluemix.net/check HTTP/1.1" "\r\n"
                        "Host: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "\r\n"
                        "$H"),
            website2,stash_size,sd);
   session = ether.tcpSend();  
}


JsonObject& root = jsonBuffer.createObject();

static void getReq () {
  byte sd = stash.create();
  /*String xa = "{\"x\":";
  String ya = ",\"y\":";
  String za = ",\"z\":";
  String zza = z + "}";
  stash.print( xa + x + ya + y + za +  zza );*/
  
  root["x"] = x;
  root["y"] = y;
  root["z"] = z;
  root.printTo(stash);
    
  stash.save();
  stash_size = stash.size();
  
  Stash::prepare(PSTR("POST https://0w3aj2.messaging.internetofthings.ibmcloud.com/api/v0002/device/types/arduino/devices/sismoarduino/events/gyro HTTP/1.1" "\r\n"
                        "Host: $F" "\r\n"
                        "Content-Type: application/json" "\r\n"
                        "Authorization: Basic dXNlLXRva2VuLWF1dGg6blJSZ0I0REh1cXZEVzdKTHFi" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "\r\n"
                        "$H"),
            website,stash_size ,sd);


  // send the packet - this also releases all stash buffers once done
  // Save the session ID so we can watch for it in the main loop.
  session = ether.tcpSend();
  
}
