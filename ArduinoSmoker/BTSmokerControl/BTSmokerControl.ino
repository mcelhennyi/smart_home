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
  ////////////////////////////////////
  //If SoftwareSerial received parse//
  ////////////////////////////////////
  if (mySerial.available()) 
  {
    while(mySerial.available())
    {
      mySerial.readBytesUntil('^', buff, buffLength);
      parseIncomingMessage(); 
    }
  }
  
  //Change target temps from input
  if(typeString == "pit" && done)
  {
    targetPitTemp = dataString.toInt();
    done = false;
  }
  if(typeString == "meat" && done)
  {
    targetMeatTemp = dataString.toInt();
    done = false; 
  }
  if(typeString == "bInc" && done)
  {
    if(brightness <= 245)
    {
      brightness += 10;  
    }
    done = false;     
  }  
  if(typeString == "bDec" && done)
  {
    if(brightness >= 10)
    {
      brightness -= 10;   
    }
    done = false;      
  }  
  if(typeString == "cInc" && done)
  {
    if(contrast <= 245)
    {
      contrast += 10;  
    }
    done = false;       
  }  
  if(typeString == "cDec" && done)
  {
    if(contrast >= 10)
    {
      contrast -= 10;   
    }
    done = false;      
  }
  
  //set brightness and contrast
  analogWrite(brightnessPin, brightness);
  analogWrite(contrastPin, contrast);
  
  //Publish the temperatures to the bluetooth and LCD every ~100 mS, then wait 100 milliseconds
  if(count == 1000)
  {
    lcd.clear();
    String pit = "Pit Temp: " ;
    String meat = "Meat Temp: " ;
    Serial.println(pit.concat(thermister_temp(analogRead(pitProbe))));
    Serial.println(meat.concat(thermister_temp(analogRead(meatProbe))));
    lcd.print(pit);
    lcd.setCursor(0, 1);
    lcd.print(meat);
    publishMeatTemp();
    publishPitTemp();
    count = 0;
  }
  count++;
  delay(1);
}

///////////////////////
//Serial/BT Functions//
///////////////////////
//When called it will parse readInMessage and set newMessage to false
  //This uses the assumption that the incomming message is on the form "id=data*^" 
    //where there is one id permessage and everything between "=" and "*" is id's data
void parseIncomingMessage()
{
  int yD = 0;
  char typeArray[messageFieldLength];
  char dataArray[messageFieldLength];
  int typeLen;
  int dataLen;
  typeString = "";
  dataString = "";

  //loop through the input message till delimiter; buffLength is buffer length
  for(int i = 0; i < buffLength; i++)
  {
   //find the = delimiter
   if(buff[i] == '=')
   {
     typeLen = i;
     
    //loop through the data part of message 
    // i + 1 skips the "=" and starts on the next character; buffLength is buffer length
    for(int x = i+1; x < buffLength; x++)
    {
     //Check for ending delimiter in buffer break for loop
     if(buff[x] == '*')
     {
       dataLen = yD;
       break;
     }
     
     //define the data for the type 
     dataArray[yD] = buff[x];//yD is the counter for data
     yD++;
    }
    break; //Reached end of message, leave loop
   }
   
   //define the type
   typeArray[i] = buff[i];//yT is the counter for type
  }
    
   //Convert arrays of type(ID) and data to strings   
   for(int x = 0; x < typeLen; x++)
   {
     typeString += typeArray[x];
   }
   for(int x = 0; x < dataLen; x++)
   {
     dataString += dataArray[x];
   }
   
   done = true;

}

/////////////////////////
//PIT RELATED FUNCTIONS//
/////////////////////////
void publishPitTemp()
{
  //Fill packet
  txPacket.packetLen = sizeof(txPacket);
  txPacket.dataEnum = PIT;
  txPacket.dataField = thermister_temp(analogRead(pitProbe))*1.0;
  
  //tx packet
  for(int i = 0; i<sizeof(DataPacket); i++) 
  {
     mySerial.write(*(&txPacket.startByte+i));
  }
}

void publishMeatTemp()
{
  //Fill packet
  txPacket.packetLen = sizeof(txPacket);
  txPacket.dataEnum = MEAT;
  txPacket.dataField = thermister_temp(analogRead(meatProbe))*1.0;
  
  //tx packet
  for(int i = 0; i<sizeof(DataPacket); i++) 
  {
     mySerial.write(*(&txPacket.startByte+i));
  }
}


//Get temperature from the thermister takes analogRead(thermister pin #) as input
int thermister_temp(int aval) 
{
      	double R, T;
      
      	// These were calculated from the thermister data sheet
      	//	A = 2.3067434E-4;
      	//	B = 2.3696596E-4;
      	//	C = 1.2636414E-7;
      	//
      	// This is the value of the other half of the voltage divider
      	//	Rknown = 22200;
      
      	// Do the log once so as not to do it 4 times in the equation
      	//	R = log(((1024/(double)aval)-1)*(double)22200);
      	R = log((1 / ((1024 / (double) aval) - 1)) * (double) 22200);
      	//lcd.print("A="); lcd.print(aval); lcd.print(" R="); lcd.print(R);
      	// Compute degrees C
      	T = (1 / ((2.3067434E-4) + (2.3696596E-4) * R + (1.2636414E-7) * R * R * R)) - 273.25;
      	// return degrees F
      	return ((int) ((T * 9.0) / 5.0 + 32.0));
}
