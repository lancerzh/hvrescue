#include <Arduino.h>
#include <SoftwareSerial.h>

//#define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  mySerial.println (x)
#else
  #define DEBUG_PRINT(x)  
#endif

SoftwareSerial mySerial(11, 10); // RX, TX
char terminatorChar = '\n';    // 定义终止字符为‘\n’ 'T'; //

// Pin Assignments
#define  DATA    PORTD // PORTD = Arduino Digital pins 0-7
#define  DATA_IN PIND
#define  DATAD   DDRD  // Data direction register for DATA port

#define  VCC     8
#define  RST12V  14    // A0, Output to level shifter for !RESET
#define  RST5V   15    // A1, Output to level shifter for !RESET
#define  XTAL1   13    // A3
#define  BUTTON  A7    // A7, Run button

#define  RDY     A6    // A6, //12, M16.PD1
#define  OE       9    // 9, M16.PD2
#define  WR      17    // A2, M16.PD3

#define  BS1     16    // 16, M16.PD4
#define  XA0     19    // A5, M16.PD5
#define  XA1     18    // A4, M16.PD6
/**
 * ATmega328 has A6, A7 PINs as analog input only, can not be used as digital PIN.
 * In four function of this program, PAGEL and BS2 will always be LOW, so could
 * wire to GND in hardwire for free two PINs which used as mySerial.
 * Read chip signature use OE/ latches data.
 */
#define  PAGEL   GND    // M16.PD7
#define  BS2     12     // M16.PA0


void enterProgMode();
void exitProgMode();
void eraseChip();
void progFuseBit(int lowByte, int highByte);
void readSignature();
void readFuseBit();

int readPort();
void loadData(int data);
void loadAddr(int address);
void loadCommand(int command);

void pulse(int PIN, bool positive, int pulses);
void waitReady();

void setRstLow();
void setRstHigh();
void setRst12V();

void help();

void setup() {
  //mySerial.setTimeout(2000);
  mySerial.begin(38400); 
  delay(100);
  help();
  
  // Set up control lines for HV parallel programming
  DATA = 0x00;  // Clear digital pins 0-7
  DATAD = 0xFF; // set digital pins 0-7 as outputs
  pinMode(VCC, OUTPUT);
  pinMode(RST12V, OUTPUT);  // signal to level shifter for +12V !RESET
  pinMode(RST5V, OUTPUT);   // signal to level shifter for +5V !RESET
  pinMode(XTAL1, OUTPUT);

  pinMode(BUTTON, INPUT);
  pinMode(RDY, INPUT);

  pinMode(OE, OUTPUT);
  pinMode(WR, OUTPUT);
  pinMode(BS1, OUTPUT);
  pinMode(BS2, OUTPUT);
  pinMode(XA0, OUTPUT);
  pinMode(XA1, OUTPUT);

  digitalWrite(VCC, LOW);
  setRstLow();
  digitalWrite(XTAL1, LOW);
  digitalWrite(OE, LOW);
  digitalWrite(WR, LOW);
  digitalWrite(BS1, LOW);
  digitalWrite(BS2, LOW);
  digitalWrite(XA0, LOW);
  digitalWrite(XA1, LOW);
  
  digitalWrite(BUTTON, HIGH);  // turn on pullup resistor
  digitalWrite(RDY, HIGH);  // turn on pullup resistor

  mySerial.println("Please enter command: device, erase, fuse lb hb or help ...");
}

void loop() {
  String p, p1, p2;
  unsigned int index, a1, a2;
  String command = "";
  while(true){
    if (mySerial.available()){ 
        //DEBUG_PRINT("Serial Data Available...");
        char buff = mySerial.read(); 
        command.concat(buff);
      if (command.length() > 20) {
        mySerial.println("Your input is :");
        mySerial.println("  '" + command + "'");
        mySerial.println("It is too long to understand, reinput please!");
        command.remove(0);
        continue;
      }
      // DEBUG_PRINT("Received Serial Data: ");
      // DEBUG_PRINT(command);
      // DEBUG_PRINT(command.length());
      // String t = String(buff, HEX);
      // DEBUG_PRINT(t);
      if(buff == '\n' || buff == '\r'){
        DEBUG_PRINT("==(n,r)");
        break;
      }
    } else {
      delay(100);
    }
  }

  command.toLowerCase();
  command.trim();
  DEBUG_PRINT(command);
  DEBUG_PRINT();
  mySerial.println();
  if (0 == command.length()){
    ; //mySerial.println();
  } else if (command.startsWith("device")){
    enterProgMode();
    readSignature();
    readFuseBit();
    exitProgMode(); 
  } else if (command.startsWith("erase")) {
    enterProgMode();
    eraseChip();
    exitProgMode();
  } else if (command.startsWith("fuse")) {
    p = command.substring(4);
    p.trim();
    index = p.indexOf(" ");
    p1 = p.substring(0, index);
    p1.trim();
    p2 = p.substring(index);
    p2.trim();
    if(p1.length() == 0 || p2.length() == 0){
      mySerial.println("Dont understand fuse parameter: " + command);
    } else {
      DEBUG_PRINT("p1= " + p1);
      DEBUG_PRINT("p2= " + p2);
      a1 = strtoul(p1.c_str(), 0, 16) & 0xFF;
      a2 = strtoul(p2.c_str(), 0, 16) & 0xFF;
      DEBUG_PRINT("a1= " + a1);
      DEBUG_PRINT("a2= " + a2);
      enterProgMode();
      progFuseBit(a1, a2);
      exitProgMode(); 
    }
  } else if (command.startsWith("help")) {
    help();
  } else {
    mySerial.println("Dont understand command: " + command);
  }
  //delay(1000); // wait for release key
}

void enterProgMode(){
  digitalWrite(XA1, LOW);
  digitalWrite(XA0, LOW);
  digitalWrite(BS1, LOW);
  digitalWrite(BS2, LOW);
  // -- digitalWrite(PAGEL, LOW);

  digitalWrite(VCC, HIGH);
  delay(1);
  setRstLow();
  pulse(XTAL1, HIGH, 10);

  digitalWrite(OE, HIGH);
  digitalWrite(WR, HIGH);
  setRst12V();
}

void exitProgMode(){
  delay(10);
  setRstLow();
  digitalWrite(VCC, LOW);
  delay(1);
  digitalWrite(XTAL1, LOW);
  digitalWrite(OE, LOW);
  digitalWrite(WR, LOW);
  digitalWrite(BS1, LOW);
  digitalWrite(BS2, LOW);
  digitalWrite(XA0, LOW);
  digitalWrite(XA1, LOW);

  DATA = 0x00;
  DATAD = 0xFF;
}

void eraseChip(){
  DATAD = 0xFF;
  delay(1);
  loadCommand(B10000000);
  pulse(WR, LOW, 1);
  waitReady();
  mySerial.println("Erase Chip done!");
}

String format2Hex(int byte){
  if (byte < 16){
    return "0x0"+String(byte, HEX);
  } else {
    return "0x" + String(byte, HEX);
  }
}

void readSignature(){
  int b0, b1, b2;
  loadCommand(B00001000);
  digitalWrite(BS1, LOW);
  loadAddr(0x00);
  b0 = readPort();
  loadAddr(0x01);
  b1 = readPort();
  loadAddr(0x02);
  b2 = readPort();

  mySerial.print("Signature: ");
  mySerial.print(format2Hex(b0));
  mySerial.print(" " + format2Hex(b1));
  mySerial.println(" " + format2Hex(b2));
}

void readFuseBit(){
  int b0, b1, b2, b3;
  loadCommand(B00000100);

  digitalWrite(BS1, LOW);
  digitalWrite(BS2, LOW);
  b0 = readPort();
  mySerial.print("Fuse low Byte: ");
  mySerial.println(format2Hex(b0));

  digitalWrite(BS1, HIGH);
  digitalWrite(BS2, HIGH);
  b1 = readPort();
  mySerial.print("Fuse high Byte: ");
  mySerial.println(format2Hex(b1));
  
  digitalWrite(BS1, LOW);
  digitalWrite(BS2, HIGH);
  b2 = readPort();
  mySerial.print("Fuse extended Byte: ");
  mySerial.println(format2Hex(b2));

  digitalWrite(BS1, HIGH);
  digitalWrite(BS2, LOW);
  b3 = readPort();
  mySerial.print("Lock Byte: ");
  mySerial.println(format2Hex(b3));
}

void progFuseBit(int lowByte, int highByte){
  DATAD = 0xFF;
  delay(1);
  loadCommand(B01000000);
  loadData(lowByte);
  digitalWrite(BS1, LOW);
  digitalWrite(BS2, LOW);
  pulse(WR, LOW, 1);
  waitReady();
  loadData(highByte);
  digitalWrite(BS1, HIGH);
  digitalWrite(BS2, LOW);
  pulse(WR, LOW, 1);
  waitReady();
  mySerial.println("Prog Fuse Bits done!");
}

int readPort(){
  int r = 0;
  DATAD = 0x00;
  digitalWrite(OE, LOW);
  delay(1);
  r = PIND;
  digitalWrite(OE, HIGH);
  DATAD = 0xFF;
  delay(1);
  return r;
}

void loadData(int data){
  digitalWrite(XA1, LOW);
  digitalWrite(XA0, HIGH);
  DATA = data;
  pulse(XTAL1, HIGH, 1);
}

void loadAddr(int address){
  digitalWrite(XA1, LOW);
  digitalWrite(XA0, LOW);
  digitalWrite(BS1, LOW);
  DATA = address;
  pulse(XTAL1, HIGH, 1);
}

void loadCommand(int command){
  digitalWrite(XA1, HIGH);
  digitalWrite(XA0, LOW);
  digitalWrite(BS1, LOW);
  DATA = command;
  pulse(XTAL1, HIGH, 1);
}

void pulse(int PIN, bool positive, int pulses){
  if (pulses <= 0) return;
  for ( int i = 0; i < pulses; i++){
    digitalWrite(PIN, positive);
    delay(1);
    digitalWrite(PIN, !positive);
    delay(1);
  }
}

void waitReady(){
  DEBUG_PRINT("analogRead(RDY):");
  DEBUG_PRINT(analogRead(RDY));
  while (analogRead(RDY) < 800){
    ;
  }
  DEBUG_PRINT("analogRead(RDY):");
  DEBUG_PRINT(analogRead(RDY));
  delay(1);
}

void setRstLow(){
  digitalWrite(RST12V, HIGH);
  digitalWrite(RST5V, LOW);
  delay(1);
}
void setRstHigh(){
  digitalWrite(RST12V, HIGH);
  digitalWrite(RST5V, HIGH);
  delay(1);
}
void setRst12V(){
  digitalWrite(RST12V, LOW);
  digitalWrite(RST5V, HIGH);
  delay(1);
}

void help(){
  mySerial.println(F("hello, to use avr rescue"));
  mySerial.println(F("suport three command: device, erase, fuse"));
  mySerial.println(F("  device will return device signature and fuse status."));
  mySerial.println(F("  erase will erase all flash."));
  mySerial.println(F("  fuse lowbyte highbyte will write low byte and high byte."));
}