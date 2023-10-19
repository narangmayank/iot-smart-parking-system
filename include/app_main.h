#pragma once

#include <Arduino.h>
#include <MFRC522.h>
#include <Servo.h>
#include <NTPClient.h>
#include <FirebaseArduino.h>
#include "pins_config.h"

void setupWifi();
void appMainInit();
void cycleLed(int count, int delay_ms);

extern String strUID;
extern unsigned int cardCount;

extern Servo myServo;
extern MFRC522 Reader;
extern NTPClient timeClient;