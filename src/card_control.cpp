#include "app_main.h"
#include "card_control.h"

// Convert byte array to String
static void byteArrayToString(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

bool getCardID() {                                           // Return true if Card is Scanned Successfully o/w returns false
  if (!Reader.PICC_IsNewCardPresent()) {                 // Check for the new Card
    return false;
  }
  if (!Reader.PICC_ReadCardSerial()) {                   // Now Card arrives, wair for it to be read Successfully
    return false;
  }

  char str[32]; 
  byte readCard[4]; 

  for (uint8_t i = 0; i < 4; i++) {                      // Get the UID and Convert it into String for further use
    readCard[i] = Reader.uid.uidByte[i]; 
    byteArrayToString(readCard, 4, str); 
    strUID = str;                                        // Holds the Card UID in String format
  }

  Serial.print("The UID of the Scanned Card is : ");     // Now Card is Readed Successfully
  Serial.println(strUID);                                // Print the UID of the Scanned Card on Serial monitor
  Reader.PICC_HaltA();                                   // Stop Reading 
  
  return true;
}

// Add card to Database
void addCard(String strUID) {
  // Holds index of the card where Null String is present in Database
  unsigned int addCount = 0;

  // Hold total number of cards available in Database
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

// Remove card from Database where counter is set
void removeCard(String strUID) {
  Firebase.setString(("Card" + String(cardCount) + "/UID") , "Null");      // Just Place the Null as a String in Card UID
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , 0);          // Update balance to 0 INR 
  Firebase.setString("Card" + String(cardCount) + "/Entry/Date","YYYY-MM-DD");      // Reset the Previous Entry Status                                                       
  Firebase.setString("Card" + String(cardCount) + "/Entry/Time","HH:MM:SS");
  Firebase.setString("Card" + String(cardCount) + "/Exit/Date","YYYY-MM-DD");       // Reset the Previous Exit Status
  Firebase.setString("Card" + String(cardCount) + "/Exit/Time","HH:MM:SS");
}

// Compare Scanned Card UID with Master Card UID
bool isMaster(String strUID) {
  if(strUID == Firebase.getString("MasterCardUID"))
    return true;                                                   // Return True if it is a Master Card else Return false
  else
    return false;
}

// Compare Scanned Card UID with every Card UID in Database
bool isCardFind(String strUID) {
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