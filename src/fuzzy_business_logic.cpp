#include "app_main.h"
#include "fuzzy_business_logic.h"

void fuzzy_business_logic() {
  //
  // Here comes our fuzzy business logic
  // It's very simple just charge as per pay as you use policy
  // Logic: Charge 30 INR per min.
  //

  String str = Firebase.getString("Card" + String(cardCount) + "/Entry/Time");
  String str1 = str;
  str = str.substring(3,5);
  int entryTime = str.toInt();
  int exitTime = timeClient.getMinutes();
  Serial.println("Your Entry Time is : " + str1 + " in HH:MM:SS Format");
  Serial.println("Your Exit  Time is : " + timeClient.getFormattedTime() + " in HH:MM:SS Format");
  
  // Generate the new balance status
  int currentBalance = Firebase.getInt("Card" + String(cardCount) + "/Balance");
  int charge = (exitTime - entryTime) * 30;
  int newBalance = currentBalance - charge;

  Serial.println("Your Current Balance is : " + String(currentBalance) + " INR");
  Serial.println("Sur Charge is : " + String(charge) + " INR");
  Serial.println("Your New Balance is : " + String(newBalance) + " INR");

  // Atlast, Update the card balance status
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , newBalance);
}