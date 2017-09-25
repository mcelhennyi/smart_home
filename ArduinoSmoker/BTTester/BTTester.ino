/**
* This program will monitor the temperature of the meat and the temperature of the pit
* This program will implement:
*    - Bluetooth
*    - Android App
*    - Software Serial for the bluetooth communication
*    - LCD display of current temperatures
*    - (TBD) PID control for temperature of pit blower motor to control pit temperature
* I/O:
*    - In: target pit temperature, target meat temperature, (TBD) start (to start elasped time)
*    - Out: current meat temperature, current pit temperature, (TBD) target meat temperature, (TBD) target pit temperature, (TBD) elapsed time
*
* Length of message must be "buffLength" or less including formatting characters, each field (ID or data) has a max of "messageFieldLength"
* A message is defined as: ID=DATA*^
*    - "=" is the delimiter to separate the id of the message and the data
*    - "*" notifys the message parser that the message is complete
*    - "^" notifys the reciever "mySerial.readBytesUntil('^', buff, buffLength);" that the transmission is done
*
*
*
*
*
* Code for the temperature conversion taken from http://hruska.us/tempmon/
*
* Ian McElhenny - mcelhenny_ian@yahoo.com
*/

//Includes
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

//Pins
int rxSS = 10;//Connect to BT TX
int txSS = 9; //Connect to BT RX
int meatProbe = 0; //Analog input outside port
int pitProbe = 1; //Analog input
int d4 = 2;
int d5 = 4;
int d6 = 7;
int d7 = 8;
int en = 11;
int rs = 12;
int contrastPin = 3; //PWM outputs
int brightnessPin = 5; //PWM outputs


//Variables
char buff[30]; //Length of message must be 30 or less including formatting characters
int buffLength = 30;
int messageFieldLength = 15; //This is the length of either ID or Data in a message
String typeString;
String dataString;
int targetMeatTemp = 0;
int targetPitTemp = 0;
int brightness = 150;
int contrast = 100;
int count = 0;
boolean done = false;

struct DataPacket
{
  byte   startByte;
  int    packetLen;
  int    dataEnum; // is this pit or meat temp
  float  dataField;
  byte   endByte;
}txPacket;

#define START_BYTE 5
#define END_BYTE   2

//data type enums
#define UNDEF 0
#define PIT   1
#define MEAT  2


//Set up functions
SoftwareSerial mySerial(rxSS, txSS); 

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


void setup() 
{
  ////////////////////////////////////
  //Set Up serial and SoftwareSerial//
  ////////////////////////////////////
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Serial Connected. Connecting SoftwareSerial Port...\n");
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  Serial.println("SoftwareSerial Connected: 9600 baud");
  ////////////////////////////////////
  
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("--Arduino BBQ!--");
  delay(5000);
  
  //init txPacket
  txPacket.startByte = START_BYTE;
  txPacket.packetLen = 0;
  txPacket.dataEnum = UNDEF;
  txPacket.dataField = 0;
  txPacket.endByte = END_BYTE;
}

void loop() 
{  
  //Publish the temperatures to the bluetooth and LCD every ~100 mS, then wait 100 milliseconds
  if(count == 1000)  //1000 means every second
  {
    sendMeatTemp(50.1);
    count = 0;
  }
  count++;
  delay(1);
}

void sendMeatTemp(float temp)
{
  //Fill packet
  txPacket.packetLen = sizeof(txPacket);
  txPacket.dataEnum = MEAT;
  txPacket.dataField = temp;
  
  //tx packet
  Serial.println("Begin");
Serial.println(txPacket.packetLen);
for( int i = 0; i<4; i++)
{
 Serial.write((byte)*(&txPacket.dataField+i)); 
   Serial.println("space");

}
  for(int i = 0; i<sizeof(DataPacket); i++) 
  {
     mySerial.write(*(&txPacket.startByte+i));
  }
//  Serial.println("End");
}


