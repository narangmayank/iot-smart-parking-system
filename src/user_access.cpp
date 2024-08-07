#include "app_main.h"
#include "fuzzy_business_logic.h"
#include "user_access.h"                  

// function to get the date from epoch time
static String getDate() {
  unsigned long epochTime = timeClient.getEpochTime();                   
  struct tm *ptm = gmtime ((time_t *)&epochTime);                    
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  return currentDate;
}

// function to open the door lock
static void openDoorLock() {
  // Indicate the status by Green LED ON
  digitalWrite(GREEN_LED, HIGH);                          
  Serial.println("Access Granted");   

  Serial.println("Door Opening...");
  for(int angle = 0; angle < 180; angle++) {                
    myServo.write(angle);
    delay(15);
  }

  Serial.println("Door Opened");
  Serial.println("You may pass now");

  // Keep the door open for 7 seconds 
  delay(7000);

  Serial.println("Door Locking...");
  for(int angle = 180; angle > 0; angle--) {
    myServo.write(angle);
    delay(15);
  }

  Serial.println("Door Locked");
  digitalWrite(GREEN_LED, LOW);
}

// function to  charge for the parking as and settle the new balance
static void charge_parking_and_settle_balance()
{
  // fuzzy business logic to 1$/hr :)
  fuzzy_business_logic();
}

// function to grant the entry
void grantEntry() {
  // Update the time client to fetch current date and time from NTP Server
  timeClient.update();
  
  // Mark the entry in the database
  Firebase.setString(("Card" + String(cardCount) + "/Entry/Date") , getDate());
  Firebase.setString(("Card" + String(cardCount) + "/Entry/Time") , timeClient.getFormattedTime());
  Firebase.setBool(("Card" + String(cardCount) + "/IsVehicleIn") , true);

  Serial.println("Please park your vehicle inside!");
  Serial.println();  

  // open the door lock
  openDoorLock();  
}

// function to grant the exit
void grantExit() {
  // Update the time client to fetch current date and time from NTP Server
  timeClient.update();

  // Mark the exit in the database
  Firebase.setString(("Card" + String(cardCount) + "/Exit/Date") , getDate());
  Firebase.setString(("Card" + String(cardCount) + "/Exit/Time") , timeClient.getFormattedTime());
  Firebase.setBool(("Card" + String(cardCount) + "/IsVehicleIn") , false);
  
  // charge for the parking as and settle the new balance
  charge_parking_and_settle_balance();

  Serial.println("Thank you for parking you vehicle!");
  Serial.println();

  // open the door lock
  openDoorLock();
}

// function to deny the access
void accessDenied() {
  // Indicate the status by Red LED ON
  digitalWrite(RED_LED, HIGH);

  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either you card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  delay(1000);

  Serial.println();
  digitalWrite(RED_LED, LOW);
}

