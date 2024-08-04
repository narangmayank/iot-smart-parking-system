#include "app_main.h"
#include "card_control.h"

// function to convert byte array to string
static void byteArrayToString(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
    buffer[i * 2 + 1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  buffer[len * 2] = '\0';
}

// function to get the card UID
bool getCardID() {
  if (!Reader.PICC_IsNewCardPresent()) {
    return false;
  }
  if (!Reader.PICC_ReadCardSerial()) {
    return false;
  }

  char str[32]; 
  byte readCard[4]; 

  // Get the UID and Convert it into String for further use
  for (uint8_t i = 0; i < 4; i++) {
    readCard[i] = Reader.uid.uidByte[i]; 
    byteArrayToString(readCard, 4, str); 
    strUID = str;
  }

  // Print the card UID for refrence
  Serial.print("The UID of the Scanned Card is : ");     
  Serial.println(strUID);
  Reader.PICC_HaltA();

  return true;
}

// function to add card to the database
void addCard(String strUID) {
  // Holds index of the card where Null String is present in Database
  unsigned int addCount = 0;

  // Hold total number of cards available in Database
  unsigned int totalCards = Firebase.getInt("TotalCards");

  // Check if Card with UID exists
  for (unsigned int i=1; i<=totalCards; i++) {
    String cardUID = Firebase.getString("Card" + String(i) + "/UID");
    if(cardUID == "Null") {
      addCount=i;
      break;
    }
  }

  if(addCount > 0) {  
    // Use the unused card from the database and make the entry
    Firebase.setString(("Card" + String(addCount) + "/UID") , strUID);

    // Giveaway initial balance of 10$
    Firebase.setInt(("Card" + String(addCount) + "/Balance") , 10*ONE_DOLLAR_INR_PRICE);
  }
  else {
    // Add a new card to the database using card template
    FirebaseObject obj(Firebase.get("CardTemplate"));
    Firebase.set(("Card" + String(totalCards+1)) , obj.getJsonVariant());  
    Firebase.setString(("Card" + String(totalCards+1) + "/UID") , strUID);
    Firebase.setInt("TotalCards" , totalCards+1);   

    // Giveaway initial balance of 10$
    Firebase.setInt(("Card" + String(totalCards+1) + "/Balance") , 10*ONE_DOLLAR_INR_PRICE);                        
  }
}

// function to remove card from the database
void removeCard(String strUID) {
  // Push the default values in the database
  Firebase.setString(("Card" + String(cardCount) + "/UID") , "Null");
  Firebase.setInt(("Card" + String(cardCount) + "/Balance") , 0);
  Firebase.setString("Card" + String(cardCount) + "/Entry/Date","YYYY-MM-DD");                                                       
  Firebase.setString("Card" + String(cardCount) + "/Entry/Time","HH:MM:SS");
  Firebase.setString("Card" + String(cardCount) + "/Exit/Date","YYYY-MM-DD");
  Firebase.setString("Card" + String(cardCount) + "/Exit/Time","HH:MM:SS");
}

// function to check if the card is Master card
bool isMaster(String strUID) {
  if(strUID == Firebase.getString("MasterCardUID"))
    return true;
  else
    return false;
}

// function to check if the card is there in database
bool isCardFind(String strUID) {
  unsigned int totalCards = Firebase.getInt("TotalCards");

  for (unsigned int i=1; i<=totalCards; i++) {
    String cardUID=Firebase.getString("Card" + String(i) + "/UID");
    if( strUID == cardUID) {
      // Set the counter to the position where card is find
      cardCount=i;
      return true;
    }
  }

  return false;
}