#include <FirebaseArduino.h> // We are using Firebase Real Time Database to store and retrieve all the data needed
#include <ESP8266WiFi.h>     // Library for NodeMCU TCP/IP Stack
#include <NTPClient.h>       // Library for NTP Server
#include <WiFiUdp.h>         // NTP Server wants UDP Connection
#include <SPI.h>             // MFRC522 uses SPI Protocol
#include <MFRC522.h>         // Library for MFRC522 Card Raeder
#include <Servo.h>           // Library for Servo Motor

#define FIREBASE_HOST  "Your Firebase Host without https://"       // Firebase Credentials
#define FIREBASE_AUTH  "Your Firebase Authentication key"     
#define WIFI_SSID      "Your Wifi Name"                            // Wifi Credentials
#define WIFI_PASSWORD  "Your Wifi Password"
#define utcOffset 19800                                            // UTC Offset for India is +5:30h which means 19800 seconds

#define servoPin D2            // Set Servo Pin
#define greenLed D0            // Set Led Pins
#define redLed   D1
#define blueLed  D4
#define slaveSelectPin D8      // Slave Select for MFRC522 (SDA Pin)
#define resetPin       D3      // Reset for MFRC522 (RST Pin)

int angle=0;                   // Set angle to rotate servo 
unsigned int cardCount=0;      // Holds index of the card if find in Database
unsigned int addCount =0;      // Holds index of the card where Null String is present in Database
bool readSuccess = false;      // Boolean variable to check whether card is readed Successfully or not
bool updateMode  = false;      // Boolean variable to switch between Update Mode & Regular Mode
byte readCard[4];              // Stores Card UID when card is readed Successfully 
char str[32];                  // Temporary character array used to convert UID byte array into String
String strUID;                 // Holds Card UID in String format 

Servo myServo;                                                 // Servo Instance
WiFiUDP udpClient;                                             // UDP Client Instance for NTP Server
MFRC522 Reader(slaveSelectPin,resetPin);                       // MFRC522 Reader Instance
NTPClient timeClient(udpClient, "pool.ntp.org", utcOffset);    // NTP Client Instance for connecting to NTP Server

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
  WiFi.begin(WIFI_SSID , WIFI_PASSWORD);                       // Connect to Acesss Point with SSID (Name) and Password
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
      readSuccess = getID();                          
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
    readSuccess = getID();                     // Keep Checking the Reader Status
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

bool getID() {                                           // Return true if Card is Scanned Successfully o/w returns false
  if (!Reader.PICC_IsNewCardPresent()) {                 // Check for the new Card
    return false;
  }
  if (!Reader.PICC_ReadCardSerial()) {                   // Now Card arrives, wair for it to be read Successfully
    return false;
  }

  Serial.print("The UID of the Scanned Card is : ");     // Now Card is Readed Successfully 

  for (uint8_t i = 0; i < 4; i++) {                      // Get the UID and Convert it into String for further use
    readCard[i] = Reader.uid.uidByte[i]; 
    byteArrayToString(readCard, 4, str); 
    strUID = str;                                        // Holds the Card UID in String format
  }
  Serial.println(strUID);                                // Print the UID of the Scanned Card on Serial monitor
  Reader.PICC_HaltA();                                   // Stop Reading 
  return true;
}

void byteArrayToString(byte array[], unsigned int len, char buffer[]) {        // Convert byte array to String
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

bool isMaster(String strUID) {                                     // Compare Scanned Card UID with Master Card UID
  if(strUID == Firebase.getString("MasterCardUID"))
    return true;                                                   // Return True if it is a Master Card else Return false
  else
    return false;
}

bool isCardFind(String strUID) {                                   // Compare Scanned Card UID with every Card UID in Database
  unsigned int totalCards = Firebase.getInt("TotalCards");
  for (unsigned int i=1; i<=totalCards; i++) {
    String cardUID=Firebase.getString("Card" + String(i) + "/UID");
    if( strUID == cardUID) {
      cardCount=i;                                                 // Set the counter to the position where card is find
      return true;                                                 // Return True if Scanned card is there in Database else Return false
    }
  }
  return false;
}

void addCard(String strUID) {                                      // Add card to Database
  unsigned int totalCards = Firebase.getInt("TotalCards");
  for (unsigned int i=1; i<=totalCards; i++) {
    String cardUID = Firebase.getString("Card" + String(i) + "/UID");
    if(cardUID == "Null") {                                        // First Check for Card with UID as Null String ("Null")
      addCount=i;                                                  // If Null UID find then Set the counter there
      break;
    }
  }
  if(addCount > 0) {                                                        // If Counter is set then add the card there
    Firebase.setString(("Card" + String(addCount) + "/UID") , strUID);      // At First set the Card UID to Scanned Card UID
    Firebase.setInt(("Card" + String(addCount) + "/Balance") , 100);        // Secondly, update the balance to 100 INR (New User Charge)
    addCount=0;                                                             // Reset the  Add Counter 
  }
  else {                                                                    // Else make a New Entry in Database right after previous Card
    FirebaseObject obj(Firebase.get("CardTemplate"));                       // At First Get the Card Template and Copy it
    Firebase.set(("Card" + String(totalCards+1)) , obj.getJsonVariant());  
    Firebase.setString(("Card" + String(totalCards+1) + "/UID") , strUID);  // Now Set the Card UID to Scanned Card UID
    Firebase.setInt(("Card" + String(totalCards+1) + "/Balance") , 100);    // Update the balance to 100 INR (New User Charge)
    Firebase.setInt("TotalCards" , totalCards+1);                           // Update the TotalCards
  }
}

void removeCard(String strUID) {                                           // Remove card from Database where counter is set
  Firebase.setString(("Card" + String(cardCount) + "/UID") , "Null");      // Just Place the Null as a String in Card UID
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , 0);          // Update balance to 0 INR 
  Firebase.setString("Card" + String(cardCount) + "/Entry/Date","YYYY-MM-DD");      // Reset the Previous Entry Status                                                       
  Firebase.setString("Card" + String(cardCount) + "/Entry/Time","HH:MM:SS");
  Firebase.setString("Card" + String(cardCount) + "/Exit/Date","YYYY-MM-DD");       // Reset the Previous Exit Status
  Firebase.setString("Card" + String(cardCount) + "/Exit/Time","HH:MM:SS");
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

void accessGranted() {                                  // Grant the Acess
  digitalWrite(greenLed,HIGH);                          // Green LED On
  Serial.println("Access Granted");                    
  Serial.println("Door Opening...");
  for(angle = 0; angle < 180; angle++) {                // Rotate Servo from 0 degree to 180 degree
    myServo.write(angle);
    delay(15);
  }
  Serial.println("Door Opened");
  Serial.println("You may pass now");
  delay(7000);                                          // Close the door after 7 seconds      
  Serial.println("Door Locking...");
  for(angle = 180; angle > 0; angle--) {                // Rotate the Servo from 180 degree to 0 degree
    myServo.write(angle);
    delay(15);
  }
  Serial.println("Door Locked");
  digitalWrite(greenLed,LOW);                           // Green LED Off
}

void accessDenied() {                                   // Acess Denied , Either card is not valid or Insufficient Balance
  digitalWrite(redLed,HIGH);                            // RED LED On
  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either you card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  Serial.println();
  delay(1000);
  digitalWrite(redLed,LOW);                               // RED LED Off
}

void grantEntry() {                                      // Grant the Entry and push the current date & time to the Database
   timeClient.update();                                  // Update the time client to fetch current date and time from NTP Server
   Firebase.setString(("Card" + String(cardCount) + "/Entry/Date") , getDate());                      // Push Current Date in YYYY-MM-DD format
   Firebase.setString(("Card" + String(cardCount) + "/Entry/Time") , timeClient.getFormattedTime());  // Push Current Time in HH:MM:SS   format
   Firebase.setBool(("Card" + String(cardCount) + "/IsPersonIn") , true);      // Mark the Entry
   Serial.println("Please Park your vehicle Inside");
   accessGranted();                                                            // Access Granted 
   Serial.println();   
}

void grantExit() {                                       // Grant the Exit and Push the current date & time to Database
  timeClient.update();                                   // Update the timer client to fetch current date and time from NTP Server
  Firebase.setString(("Card" + String(cardCount) + "/Exit/Date") , getDate());                        // Push Current Date in YYYY-MM-DD format
  Firebase.setString(("Card" + String(cardCount) + "/Exit/Time") , timeClient.getFormattedTime());    // Push Current Time in HH:MM:SS format
  
  String str=Firebase.getString("Card" + String(cardCount) + "/Entry/Time");       // Get the Entry Time from Database for current user 
  String str1=str;
  str=str.substring(3,5);                                                          // We are Interested in Minutes only (HH:MM:SS)
  int entryTime=str.toInt();                                                       // Convert it into Integer
  int exitTime=timeClient.getMinutes();                                            // Get the Exit Time
  Serial.println("Your Entry Time is : " + str1 + " in HH:MM:SS Format");
  Serial.println("Your Exit  Time is : " + timeClient.getFormattedTime() + " in HH:MM:SS Format");
  
  int currentBalance = Firebase.getInt("Card" + String(cardCount) + "/Balance");   // Get the Current balance in card from Database
  int charge = (exitTime-entryTime)*30;                                            // Calculate the charge as 30 INR per Minute
  int newBalance = currentBalance - charge;                                        // Calculate new balance
  Serial.println("Thank you for choosing our service");
  Serial.println("We wish you a great day ahead");
  Serial.println("Your Current Balance is : " + String(currentBalance) + " INR");
  Serial.println("Your Charge is : " + String(charge) + " INR");
  Serial.println("Your New Balance is     : " + String(newBalance) + " INR");
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , newBalance);         // Update the New Balance
  Firebase.setBool(("Card" + String(cardCount) + "/IsPersonIn") , false);          // Erase the Entry Mark
  accessGranted();                                                                 // Access Granted 
  Serial.println();
}

String getDate() {                                                                 // Getting Current Date from epoch time 
  unsigned long epochTime = timeClient.getEpochTime();                   
  struct tm *ptm = gmtime ((time_t *)&epochTime);                    
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  return currentDate;                                                              // Return Current date as String
}
