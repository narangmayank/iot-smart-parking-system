#pragma once

#include <Arduino.h>

bool getCardID();
void addCard(String strUID);
void removeCard(String strUID);
bool isMaster(String strUID);
bool isCardFind(String strUID);