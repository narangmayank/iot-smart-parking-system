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

unsigned int cardCount = 0;    // Holds index of the card if find in Database
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
MFRC522 Reader(MFRC522_SLAVE_SELECT_PIN,MFRC522_RESET_PIN);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
 
  setupWifi();
  appMainInit();
  cycleLed(2, 200);

  Serial.println("|************ IoT Smart Park System ************|");
  Serial.println();
  Serial.println("Fw Version : " + String(FIRMWARE_VERSION) + " | Hw Version : " + String(HARDWARE_VERSION));
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

      // Visualize the no master status by blinking Blue LED super quickly      
      digitalWrite(BLUE_LED, HIGH);  
      delay(250);
      digitalWrite(BLUE_LED, LOW);
      delay(250);

    } while(!readSuccess); // Block till Card read  

    // Card readed successfully, So make it Master Card and push all the neccessary data  
    // Basically Creating the card template over firebase for further cards entry and exit                                                   
    Firebase.setInt("TotalCards", 0);                   // TotalCards to 0
    Firebase.setString("MasterCardUID", strUID);        // MasterCardUID to Scanned Card UID
    Firebase.setBool("IsMasterDefined", true);          // IsMasterDefined to True

    // Card Template
    Firebase.setInt("CardTemplate/Balance", 0);        
    Firebase.setString("CardTemplate/UID", "XXXXXXXX");                                      
    Firebase.setString("CardTemplate/Entry/Date", "YYYY-MM-DD");                                                       
    Firebase.setString("CardTemplate/Entry/Time", "HH:MM:SS");
    Firebase.setString("CardTemplate/Exit/Date", "YYYY-MM-DD");
    Firebase.setString("CardTemplate/Exit/Time", "HH:MM:SS");
    Firebase.setBool("CardTemplate/IsVehicleIn", false);
        
    Serial.println("Welcome Master");
    Serial.println("Initialization Completed !");
    Serial.println();
    
    // Visualize the Initialization by cycle the LED's Blinks
    cycleLed(4, 200);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  //
  // Main loop first will wait for any card to be scanned
  // Then based on card (Master Card or Normal Card) it will enter into desired mode 
  // Then based on mode the code will do the needfull :)
  //

  do {
    readSuccess = getCardID();
    delay(200);
  }while(!readSuccess); // Block till Card read

  //
  // Update Mode : You can manage the cards.
  // Regular Mode : You can mark the entry/exit in the parikng.
  //

  if(updateMode) {     
    // Visualize Update Mode by Turn OFF Blue LED
    digitalWrite(BLUE_LED, LOW);

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
        digitalWrite(RED_LED, HIGH);   
        removeCard(strUID);
        Serial.println("Card Removed");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(RED_LED, LOW);
      }
      else {
        // If scanned card is not in the databse then add it     
        Serial.println("I don't know this card , adding..."); 

        // Visualize it by making Green LED On till the card is added to database                      
        digitalWrite(GREEN_LED, HIGH);              
        addCard(strUID);
        Serial.println("Card Added");
        Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
        Serial.println("Scan Master Card again to exit Update mode");
        Serial.println();
        digitalWrite(GREEN_LED, LOW);
      }
    }
  }
  else {
    // Visualize Regular Mode by Turn ON Blue LED        
    digitalWrite(BLUE_LED,  HIGH);

    if(isMaster(strUID)) {
      // If scanned card is Master Card, Step into Update Mode 
      Serial.println("Hey Master!");
      Serial.println("Entering Update Mode...");          // For Debugging Purposes
      Serial.println("Scan a unknown card to Add to Database or Scan a known card to Remove from Database");
      Serial.println("Scan Master card again to exit Update Mode");
      Serial.println();
      updateMode = true;
    }
    else {
      // If scanned card exists then check for the balance and grant the extry     
      if(isCardFind(strUID)==true) {

        //
        // Allow only if card have some threshhold (Above 1$)
        // We will let the card be in negative if in case the person spends more time.
        //

        if(Firebase.getInt("Card" + String(cardCount) + "/Balance") >= ONE_DOLLAR_INR_PRICE) {
          if(Firebase.getBool("Card" + String(cardCount) + "/IsVehicleIn")) {
            // Vehicle is already inside, Grant the Exit      
            grantExit();
          }
          else {
            // Vehicle is not inside, Grant the Entry
            grantEntry();
          }
        }
        else {
          // Card doesn't have sufficient Balance
          Serial.println("Card doesn't have sufficient balance, Please recharge it.");
          accessDenied();
        }
      } 
      else {
        // Card doesn't exists
        Serial.println("Card doesn't exists, Please add it.");
        accessDenied();
      }
    }
  } 
}

// function to cycle the leds
void cycleLed(int count, int delay_ms) {   
  for(int i=0; i<count; i++) {
    // Green LED On for 200 us                             
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED,   LOW );   
    digitalWrite(BLUE_LED,  LOW );   
    delay(delay_ms);

    // Blue LED On for 200 us
    digitalWrite(GREEN_LED, LOW );                              
    digitalWrite(RED_LED,   LOW );  
    digitalWrite(BLUE_LED,  HIGH);  
    delay(delay_ms);

    // Red LED On for 200 us
    digitalWrite(GREEN_LED, LOW );   
    digitalWrite(RED_LED,   HIGH); 
    digitalWrite(BLUE_LED,  LOW );   
    delay(delay_ms);
  } 
}

// function to setup the wifi with predefined credentials
void setupWifi() {
  // set the wifi to station mode to connect to a access point
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
}

// function to initiaize the main app
void appMainInit()
{
  //
  // MFRC522 Reader is connected via SPI
  // initialize the MFRC522 Card Reader
  //
  SPI.begin();                
  Reader.PCD_Init();     

  // setup the pins (servo + led)
  myServo.attach(SERVO_PIN);   
  pinMode(GREEN_LED, OUTPUT);    
  pinMode(RED_LED ,  OUTPUT);
  pinMode(BLUE_LED,  OUTPUT);

  // default configurations (servo + led)
  myServo.write(0); 
  digitalWrite(GREEN_LED, LOW);  
  digitalWrite(RED_LED ,  LOW);
  digitalWrite(BLUE_LED,  LOW); 

  // sync the current time from SNTP Server
  timeClient.begin();

  // Atlast. Connect to Firebase using Host and Authentication key
  Firebase.begin(FIREBASE_HOST , FIREBASE_AUTH);
}