#include "app_main.h"
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
  digitalWrite(greenLed,HIGH);                          
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
  digitalWrite(greenLed,LOW);
}

// function to grant the entry
void grantEntry() {
  // Update the time client to fetch current date and time from NTP Server
  timeClient.update();
  
  // Mark the entry in the database
  Firebase.setString(("Card" + String(cardCount) + "/Entry/Date") , getDate());
  Firebase.setString(("Card" + String(cardCount) + "/Entry/Time") , timeClient.getFormattedTime());
  Firebase.setBool(("Card" + String(cardCount) + "/IsVehicleIn") , true);

  Serial.println("Please Park your vehicle inside");
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
  
  //
  // Here comes our business logic
  // It's very simple just charge as per pay as you use policy
  // Logic: Charge 30INR per min.
  //

  String str=Firebase.getString("Card" + String(cardCount) + "/Entry/Time");
  String str1=str;
  str=str.substring(3,5);
  int entryTime=str.toInt();
  int exitTime=timeClient.getMinutes();
  Serial.println("Your Entry Time is : " + str1 + " in HH:MM:SS Format");
  Serial.println("Your Exit  Time is : " + timeClient.getFormattedTime() + " in HH:MM:SS Format");
  
  // Generate the new balance status
  int currentBalance = Firebase.getInt("Card" + String(cardCount) + "/Balance");
  int charge = (exitTime - entryTime) * 30;
  int newBalance = currentBalance - charge;

  // Update the card status back to database
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , newBalance);
  Firebase.setBool(("Card" + String(cardCount) + "/IsVehicleIn") , false);

  Serial.println("Your Current Balance is : " + String(currentBalance) + " INR");
  Serial.println("Sur Charge is : " + String(charge) + " INR");
  Serial.println("Your New Balance is : " + String(newBalance) + " INR");

  Serial.println("Thank you for choosing our service");
  Serial.println("We wish you a great day ahead");
  Serial.println();

  // open the door lock
  openDoorLock();
}

// function to deny the access
void accessDenied() {
  // Indicate the status by Red LED ON
  digitalWrite(redLed,HIGH);

  Serial.println("Access Denied");
  Serial.println("You can't go ahead");
  Serial.println("Either you card is not valid or you don't have sufficient balance in card");
  Serial.println("Please contact customer care");
  delay(1000);

  Serial.println();
  digitalWrite(redLed,LOW);
}