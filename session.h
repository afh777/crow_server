#pragma once

#include <string>
#include <iostream>

using namespace std;

string createSession(string username);

bool sessionExists(string sessionID);

string getUsername(string sessionID);

void deleteSession(string sessionID);
