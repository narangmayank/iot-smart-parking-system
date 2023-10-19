#include <SPI.h>
#include <Servo.h>
#include <MFRC522.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FirebaseArduino.h>

#include "app_main.h"
#include "user_access.h"
#include "card_control.h"
#include "arduino_secrets.h"

// secret configurations (wifi + firebase)
#define WIFI_SSID      SECRET_WIFI_SSID
#define WIFI_PASS      SECRET_WIFI_PASS
#define FIREBASE_HOST  SECRET_FIREBASE_HOST
#define FIREBASE_AUTH  SECRET_FIREBASE_AUTH

// UTC Offset for India is +5:30h which means 19800 seconds
#define UTC_OFFSET_INDIA_IN_SEC 19800

unsigned int cardCount = 0;      // Holds index of the card if find in Database
bool readSuccess = false;      // Boolean variable to check whether card is readed Successfully or not
bool updateMode  = false;      // Boolean variable to switch between Update Mode & Regular Mode
String strUID;                 // Holds Card UID in String format 

// UDP Client Instance for NTP Server
WiFiUDP udpClient;

// NTP Client Instance for connecting to NTP Server
NTPClient timeClient(udpClient, "pool.ntp.org", UTC_OFFSET_INDIA_IN_SEC);

// Servo Motor Instance
Servo myServo;                       

// MFRC522 Reader Instance
MFRC522 Reader(slaveSelectPin,resetPin);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  SPI.begin();                
  Reader.PCD_Init();  
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID , WIFI_PASS);

  Serial.println();
  Serial.print("Connecting to " + String(WIFI_SSID));

  // wait till chip is being connected to wifi  (Blocking Mode)
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
  }

  // now it is connected to the access point just print the ip assigned to chip
  Serial.println();
  Serial.print("Connected to " + String(WIFI_SSID) + ", Got IP address : ");
  Serial.println(WiFi.localIP());

  myServo.attach(servoPin);   
  pinMode(greenLed,OUTPUT);    
  pinMode(redLed , OUTPUT);
  pinMode(blueLed, OUTPUT);

  myServo.write(0); 
  digitalWrite(greenLed,LOW);  
  digitalWrite(redLed , LOW);
  digitalWrite(blueLed, LOW); 

  timeClient.begin();
  Firebase.begin(FIREBASE_HOST , FIREBASE_AUTH);

  Serial.println("|************ IoT Smart Park System ************|");
  Serial.println();

  // Desciption of our Smart Park System 
  Serial.println("It works in two modes i.e (Regular Mode[Blue LED On] and Update Mode[Blue LED Off])");
  Serial.println("1) Regular Mode : If Card is Valid then Grant the Entry or Exit else deny the Access");
  Serial.println("2) Update Mode  : ADD the Unknown Cards to Database and remove the known Cards from Database"); 
  Serial.println("Default Mode is Regular Mode, If you want to go to Update Mode then just scan the Master Card");
  Serial.println("If you want to go back to the Regualar Mode then just scan the Master Card again"); 
  Serial.println("Thank You, Happy Usage..."); 
  Serial.println();

  // Check if Master Card is defined of not (Do not further if Master is not defined)   
  if(!Firebase.getBool("IsMasterDefined")) {
    Serial.println("There is no Master");
    Serial.println("Please Scan a Card to define Master Card...");

    do {       
      readSuccess = getCardID();                          
      digitalWrite(blueLed,HIGH);  
      delay(300);
      digitalWrite(blueLed,LOW);
      delay(300);
    } while(!readSuccess); // Block till Card read  

    // Card readed successfully, So make it Master Card and push all the neccessary data  
    // Basically Creating the card template over firebase for further cards entry and exit                                                   
    Firebase.setInt("TotalCards", 0);                   // TotalCards to 0
    Firebase.setString("MasterCardUID", strUID);        // MasterCardUID to Scanned Card UID
    Firebase.setBool("IsMasterDefined", true);          // IsMasterDefined to True

    // Card Template
    Firebase.setInt("CardTemplate/Balance", 0);        
    Firebase.setString("CardTemplate/UID", "");                                      
    Firebase.setString("CardTemplate/Entry/Date", "YYYY-MM-DD");                                                       
    Firebase.setString("CardTemplate/Entry/Time", "HH:MM:SS");
    Firebase.setString("CardTemplate/Exit/Date", "YYYY-MM-DD");
    Firebase.setString("CardTemplate/Exit/Time", "HH:MM:SS");
    Firebase.setBool("CardTemplate/IsVehicleIn", false);
        
    Serial.println("Welcome Master");
    Serial.println("Initialization Completed !");
    Serial.println();
    
    // Visualize the Initialization by cycle the LED's Blinks
    cycleLed();
    cycleLed();
    cycleLed();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  do {
    readSuccess = getCardID();                   
    if(updateMode) {
      // Visualize Update Mode by Turn Off all the LED's                         
      digitalWrite(greenLed,LOW);
      digitalWrite(redLed,  LOW);
      digitalWrite(blueLed, LOW);
    }
    else {
      // Visualize Regular Mode by Turn on only Blue LED
      digitalWrite(greenLed,LOW);              
      digitalWrite(redLed,  LOW);
      digitalWrite(blueLed,HIGH);
    }
    delay(200);
  }while(!readSuccess); // Block till Card read  

  //
  // Update Mode : You can add/remove the cards.
  // Regular Mode : You can mark the entry/exit in the paring.  
  //

  if(updateMode) {                                 
    if(isMaster(strUID)) {
      // Exit the Update Mode if Master Card is Scanned again                         
      Serial.println("Hey Master!");
      Serial.println("Exiting Update Mode...");
      Serial.println();
      updateMode = false;
    }
    else {
      if(isCardFind(strUID)) {
        // If scanned card is there in database then remove it           
        Serial.println("I know this card , removing...");

        // Visualize it by making Red LED On till the Card is removed from database  
        digitalWrite(redLed,HIGH);   
        removeCard(strUID);
        Serial.println("Card Removed");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(redLed,LOW);
      }
      else {
        // If scanned card is not in the databse then add it     
        Serial.println("I don't know this card , adding..."); 

        // Visualize it by making Green LED On till the card is added to database                      
        digitalWrite(greenLed,HIGH);              
        addCard(strUID);
        Serial.println("Card Added");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(greenLed,LOW);
      }
    }
  }
  else {
    if(isMaster(strUID)) {
      // If Scanned Card is Master Card, Step into Update Mode 
      Serial.println("Hey Master!");
      Serial.println("Entering Update Mode...");          // For Debugging Purposes
      Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
      Serial.println("Scan Master card again to exit Update Mode");
      Serial.println();
      updateMode = true;
    }
    else {
      // If Scanned Card is Valid and Balance in the Card is atleast 30 INR then Grant the Entry and Exit       
      if(isCardFind(strUID)==true && Firebase.getInt("Card" + String(cardCount) + "/Balance") >= 30) {
        // Check for the status of the Vehicle
        if(Firebase.getBool("Card" + String(cardCount) + "/IsVehicleIn")) { 
          // Vehicle is inside, Grant the Exit      
          grantExit();
        }
        else {
          // Vehicle is not inside, Grant the Entry
          grantEntry();
        }   
      }                             
      else {
        // Either Card is not valid or Insufficient Balance
        accessDenied();
      }
    }
  } 
}

// function to cycle the leds
void cycleLed() {              
  // Green LED On for 200 us                             
  digitalWrite(greenLed, HIGH);
  digitalWrite(redLed,   LOW );   
  digitalWrite(blueLed,  LOW );   
  delay(200);

  // Blue LED On for 200 us
  digitalWrite(greenLed, LOW );                              
  digitalWrite(redLed,   LOW );  
  digitalWrite(blueLed,  HIGH);  
  delay(200);

  // Red LED On for 200 us
  digitalWrite(greenLed, LOW );   
  digitalWrite(redLed,   HIGH); 
  digitalWrite(blueLed,  LOW );   
  delay(200);
}
