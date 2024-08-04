#include "app_main.h"
#include "fuzzy_business_logic.h"

void fuzzy_business_logic() {
  //
  // Here comes our fuzzy business logic
  // It's very simple just charge as per pay as you use policy
  // Logic: Charge 1$ per hour
  //

  String str_1 = Firebase.getString("Card" + String(cardCount) + "/Entry/Time");
  str_1 = str_1.substring(0,2);
  int entryTimeHrs = str_1.toInt();
  Serial.println("Your Entry Time is : " + str_1 + " in HH:MM:SS Format");

  String str_2 = Firebase.getString("Card" + String(cardCount) + "/Exit/Time");
  str_2 = str_2.substring(0,2);
  int exitTimeHrs = str_2.toInt();
  Serial.println("Your Exit  Time is : " + str_2 + " in HH:MM:SS Format");

  // Generate the new balance status
  int currentBalance = Firebase.getInt("Card" + String(cardCount) + "/Balance");
  int charge = (exitTimeHrs - entryTimeHrs) * ONE_DOLLAR_INR_PRICE;
  int newBalance = currentBalance - charge;

  Serial.println("Your Current Balance is : " + String(currentBalance) + " INR");
  Serial.println("Sur Charge is : " + String(charge) + " INR");
  Serial.println("Your New Balance is : " + String(newBalance) + " INR");

  // Atlast, Update the card balance status
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , newBalance);
}