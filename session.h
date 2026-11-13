#pragma once

#include <string>
#include <iostream>
#include <random>
#include <cstdint>
#include <array>
#include <cstring>

using namespace std;

string createSession(string username);

string encoding(array<uint8_t, 16> bytes);

array<uint8_t, 16> generateRandomBytes();

bool sessionExists(string sessionID);

string getUsername(string sessionID);

void deleteSession(string sessionID);
