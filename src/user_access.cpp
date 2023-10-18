#include "app_main.h"
#include "user_access.h"                  

// Getting Current Date from epoch time 
static String getDate() {
  unsigned long epochTime = timeClient.getEpochTime();                   
  struct tm *ptm = gmtime ((time_t *)&epochTime);                    
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  return currentDate;                                                              // Return Current date as String
}

// Grant the Acess
void accessGranted() {
  digitalWrite(greenLed,HIGH);                          // Green LED On
  Serial.println("Access Granted");                    
  Serial.println("Door Opening...");
  for(int angle = 0; angle < 180; angle++) {                // Rotate Servo from 0 degree to 180 degree
    myServo.write(angle);
    delay(15);
  }
  Serial.println("Door Opened");
  Serial.println("You may pass now");
  delay(7000);                                          // Close the door after 7 seconds      
  Serial.println("Door Locking...");
  for(int angle = 180; angle > 0; angle--) {                // Rotate the Servo from 180 degree to 0 degree
    myServo.write(angle);
    delay(15);
  }
  Serial.println("Door Locked");
  digitalWrite(greenLed,LOW);                           // Green LED Off
}

// Acess Denied , Either card is not valid or Insufficient Balance
void accessDenied() {
  digitalWrite(redLed,HIGH);                            // RED LED On
  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either you card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  Serial.println();
  delay(1000);
  digitalWrite(redLed,LOW);                               // RED LED Off
}

// Grant the Entry and push the current date & time to the Database
void grantEntry() {
   timeClient.update();                                  // Update the time client to fetch current date and time from NTP Server
   Firebase.setString(("Card" + String(cardCount) + "/Entry/Date") , getDate());                      // Push Current Date in YYYY-MM-DD format
   Firebase.setString(("Card" + String(cardCount) + "/Entry/Time") , timeClient.getFormattedTime());  // Push Current Time in HH:MM:SS   format
   Firebase.setBool(("Card" + String(cardCount) + "/IsPersonIn") , true);      // Mark the Entry
   Serial.println("Please Park your vehicle Inside");
   accessGranted();                                                            // Access Granted 
   Serial.println();   
}

// Grant the Exit and Push the current date & time to Database
void grantExit() {
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