#pragma once

#include <string>
#include <iostream>


using namespace std;

void initDatabase();

bool userExists(std::string username);

bool addUser(
	string username,
	string passwordHask);

string getPasswordHash(
		string username);
