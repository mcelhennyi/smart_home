#define ESPALEXA_ASYNC //it is important to define this before #include <Espalexa.h>!
#include <Espalexa.h>

#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "Credentials.h"

// prototypes
boolean connectWifi();

//callback functions
void onTvUnderlightChanged(uint8_t brightness, uint32_t rgb);
void onTvBacklightChanged(uint8_t brightness, uint32_t rgb);

boolean wifiConnected = false;

Espalexa espalexa;
AsyncWebServer server(80);

String device_0_name = "TV Underlight";
String device_1_name = "TV Backlight";

void setup()
{
  Serial.begin(115200);
  // Initialise wifi connection
  wifiConnected = connectWifi();

  
  if(wifiConnected){
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", getLightStatus());
    });
    server.on("/test1", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/plain", "This is a second subpage you may have.");
    });
    server.onNotFound([](AsyncWebServerRequest *request){
      if (!espalexa.handleAlexaApiCall(request)) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        request->send(404, "text/plain", "Not found");
      }
    });

    // Define your devices here.
    espalexa.addDevice(device_0_name, onTvUnderlightChanged); //simplest definition, default state off
    espalexa.addDevice(device_1_name, onTvBacklightChanged); //simplest definition, default state off

    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own
    //server.begin(); //omit this since it will be done by espalexa.begin(&server)
  } else
  {
    while (1)
    {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }
}
 
void loop()
{
   espalexa.loop();
   delay(1);
}

//our callback functions
void onTvUnderlightChanged(uint8_t brightness, uint32_t rgb) 
{
    Serial.print("onTvUnderlightChanged changed to ");
    
  //do what you need to do here, for example control RGB LED strip
  Serial.print("Brightness: ");
  Serial.print(brightness);
  Serial.print(", Red: ");
  Serial.print((rgb >> 16) & 0xFF); //get red component
  Serial.print(", Green: ");
  Serial.print((rgb >>  8) & 0xFF); //get green
  Serial.print(", Blue: ");
  Serial.println(rgb & 0xFF); //get blue
}

//our callback functions
void onTvBacklightChanged(uint8_t brightness, uint32_t rgb) 
{
    Serial.print("onTvBacklightChanged changed to ");
    
  //do what you need to do here, for example control RGB LED strip
  Serial.print("Brightness: ");
  Serial.print(brightness);
  Serial.print(", Red: ");
  Serial.print((rgb >> 16) & 0xFF); //get red component
  Serial.print(", Green: ");
  Serial.print((rgb >>  8) & 0xFF); //get green
  Serial.print(", Blue: ");
  Serial.println(rgb & 0xFF); //get blue
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  delay(100);
  return state;
}

String getLightStatus()
{
  Serial.println("New status!");

  // TODO:  Generate a color opposite of background for text
  String ret = String(" \
        <!DOCTYPE html> \
        <html> \
        <head> \
        <META HTTP-EQUIV=\"refresh\" CONTENT=\"3\"> \
        <style> \
        label.device_0 { \
          color: black; \n\
          background-color: rgb(") + espalexa.getDevice(0)->getR() + String(",") + espalexa.getDevice(0)->getG() + String(",") + espalexa.getDevice(0)->getB() + String("); \
        } \
        label.device_1 { \
          color: black; \n\
          background-color: rgb(") + espalexa.getDevice(1)->getR() + String(",") + espalexa.getDevice(1)->getG() + String(",") + espalexa.getDevice(1)->getB() + String("); \
        } \
        </head> \
        </style> \
        <body> \
        \
        <h1>Entertainment Center Light Controls</h1> \
        <label class=\'device_0\' > " + device_0_name + " Status </label> \
        <br> \
        <label class=\'device_1\' > " + device_1_name + " Status </label> \
         \
        </body> \
        </html> \
        ");
  return ret;
}
