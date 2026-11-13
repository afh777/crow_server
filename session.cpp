#include "session.h"
#include <iostream>
#include <string>
#include <unordered_map>

using namespace std;

unordered_map<
	string,
	string> sessions;

int nextSessionID = 1;

string  createSession(string username) {
	string sessionID = "session" + to_string(nextSessionID++);

	sessions[sessionID] = username;

	return sessionID;
}

bool sessionExists(string sessionID) {
	return sessions.find(sessionID) != sessions.end();
}


                                                                                                                                                                                                                                                                                                                                                                                                                                                               
