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
  Serial.begin(115200);        // Initialize Serial Communication between NodeMCU and PC at 115200 bps
  SPI.begin();                 // Initialize SPI Bus for SPI Communication between NodeMCU and MFRC522 Reader
  Reader.PCD_Init();           // Initialize the MFRC522 Card Reader
     
  myServo.attach(servoPin);    // Servo attachment decalaration
  pinMode(greenLed,OUTPUT);    // Set all the Led's to OUTPUT
  pinMode(redLed , OUTPUT);
  pinMode(blueLed, OUTPUT);
  digitalWrite(greenLed,LOW);  // Make Sure all the LED's are off Initially 
  digitalWrite(redLed , LOW);
  digitalWrite(blueLed, LOW); 
  myServo.write(0);            // Set the Servo to 0 degree
  
  WiFi.mode(WIFI_STA);                                         // Set the Wifi to Station Mode to connect to a Access Point
  WiFi.begin(WIFI_SSID , WIFI_PASS);                           // Connect to Acesss Point with SSID (Name) and Password
  Serial.print("Connecting to " + String(WIFI_SSID));          // Wait till NodeMCU is being connected to Wifi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Connected to " + String(WIFI_SSID) + ", Got IP address : ");   // Now it is coneccted to Access Point
  Serial.println(WiFi.localIP());                                              // Print the IP Assigned to NodeMCU on Serial Monitor
  Serial.println();

  Serial.println("Smart Park System With RFID");                               // For Debugging purpose  
  Serial.println("Project from Mayank Submitted to Sh. Navdeep Khatri {SIET Nilokheri}");
  Serial.println();

  Firebase.begin(FIREBASE_HOST , FIREBASE_AUTH);       // Connect to Firebase Real Time Database with Host and Authentication key
  timeClient.begin();                                  // Connect to NTP Server (No need of authentication here as it is used publically)

  if(!Firebase.getBool("IsMasterDefined")) {           // Check if Master Card is defined of not (Do not further if Master is not defined)   
    Serial.println("There is no Master");
    Serial.println("Please Scan a Card to define Master Card...");
    do {                                               // Wait till Card arrives 
      readSuccess = getCardID();                          
      digitalWrite(blueLed,HIGH);                      // Visualize it by blinking Blue LED   
      delay(300);
      digitalWrite(blueLed,LOW);
      delay(300);
    }while(!readSuccess);

    // Now Card is readed Successfully, So make it Master Card and Push all the neccessary data to the Database                                                     
    Firebase.setInt("TotalCards",0);                   // TotalCards to 0
    Firebase.setString("MasterCardUID",strUID);        // MasterCardUID to Scanned Card UID
    Firebase.setBool("IsMasterDefined",true);          // IsMasterDefined to True
    Firebase.setInt("CardTemplate/Balance",0);         // Make a Template for Cards 
    Firebase.setString("CardTemplate/UID","");                                      
    Firebase.setString("CardTemplate/Entry/Date","YYYY-MM-DD");                                                       
    Firebase.setString("CardTemplate/Entry/Time","HH:MM:SS");
    Firebase.setString("CardTemplate/Exit/Date","YYYY-MM-DD");
    Firebase.setString("CardTemplate/Exit/Time","HH:MM:SS");
    Firebase.setBool("CardTemplate/IsPersonIn",false);
        
    Serial.println("Welcome Master");                  // For Debugging purpose
    Serial.println("Initialization Completed !");
    Serial.println();
    
    cycleLed();                                        // Visualize the Initialization by cycle the LED's Blinks
    cycleLed();
    cycleLed();
  }
  // Desciption of our Smart Park System 
  Serial.println("It works in two modes i.e (Regular Mode[Blue LED On] and Update Mode[Blue LED Off])");
  Serial.println("1) Regular Mode : If Card is Valid then Grant the Entry or Exit else deny the Access");
  Serial.println("2) Update Mode  : ADD the Unknown Cards to Database and remove the known Cards from Database"); 
  Serial.println("Default Mode is Regular Mode, If you want to go to Update Mode then just scan the Master Card");
  Serial.println("If you want to go back to the Regualar Mode then just scan the Master Card again"); 
  Serial.println("Thank You, Happy Usage..."); 
  Serial.println();
}

void loop() {
  do {                                         // Wait for the Card to be scanned successfully
    readSuccess = getCardID();                     // Keep Checking the Reader Status
    delay(200);
    if(updateMode) {                           // Visualize Update Mode by Turn Off all the LED's
      digitalWrite(greenLed,LOW);
      digitalWrite(redLed,  LOW);
      digitalWrite(blueLed, LOW);
    }
    else {
      digitalWrite(greenLed,LOW);              // Visualize Regular Mode by Turn on only Blue LED
      digitalWrite(redLed,  LOW);
      digitalWrite(blueLed,HIGH);
    }
  }while(!readSuccess);

  if(updateMode) {                                 // Update Mode 
    if(isMaster(strUID)) {                         // Exit the Update Mode if Master Card is Scanned again
      Serial.println("Master Card Scanned");       // For Debugging Purposes
      Serial.println("Exiting Update Mode...");
      Serial.println();
      updateMode=false;                            // Go back to Regular Mode
      return;                                      // Do not go further and return to main loop and take a new start
    }
    else {
      if(isCardFind(strUID)) {                    // If Scanned Card is already there in Database then remove it from Database
        digitalWrite(redLed,HIGH);                // Visualize it by making Red LED On till Card is removing from Database
        Serial.println("I know this card , removing...");
        removeCard(strUID);
        Serial.println("Card Removed");           // For Debugging Purposes
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(redLed,LOW);
      }
      else {                                      // If Scanned Card is not in the Databse then add it to Database
        digitalWrite(greenLed,HIGH);              // Visualize it by making Green LED On till Card is adding to Database
        Serial.println("I don't know this card , adding...");
        addCard(strUID);
        Serial.println("Card Added");             // For Debugging Purposes
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(greenLed,LOW);
      }
    }
  }
  else {                                                  // Regular Mode
    if(isMaster(strUID)) {                                // If Scanned Card is Master Card , Change the Mode 
      updateMode=true;                                    // Go to Master Mode
      Serial.println("Entering Update Mode...");          // For Debugging Purposes
      Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
      Serial.println("Scan Master card again to exit Update Mode");
      Serial.println();
    }
    // If Scanned Card is Valid and Balance in Scanned Card is atleast 30 INR then Grant the Entry and Exit
    else {          
      if(isCardFind(strUID)==true && Firebase.getInt("Card" + String(cardCount) + "/Balance") >= 30) {
        if(Firebase.getBool("Card" + String(cardCount) + "/IsPersonIn")) {       // Check for the Entry of the Person
          grantExit();                                                           // Person is already Entered, So Grant the Exit
        }
        else {
          grantEntry();                                                          // Person is not Entered yet, So Grant the Entry
        }   
      }                             
      else {
        accessDenied();                                                          // Either Card is not valid or Insufficient Balance
      }
    }
  } 
}

void cycleLed() {                                            // Initialization Status by cycling LED's Blink
  digitalWrite(greenLed, HIGH);                              // Green LED On for 200 us
  digitalWrite(redLed,   LOW );   
  digitalWrite(blueLed,  LOW );   
  delay(200);
  digitalWrite(greenLed, LOW );                              // Blue LED On for 200 us
  digitalWrite(redLed,   LOW );  
  digitalWrite(blueLed,  HIGH);  
  delay(200);
  digitalWrite(greenLed, LOW );                              // Red LED On for 200 us   
  digitalWrite(redLed,   HIGH); 
  digitalWrite(blueLed,  LOW );   
  delay(200);
}
