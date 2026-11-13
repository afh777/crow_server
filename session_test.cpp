#include "session.h"
#include <cassert>
#include <iostream>

int main() {
	string alice1 = createSession("alice");
	string alice2 = createSession("alice");
	string bob1 = createSession("bob");


	assert(!sessionID.empty());
	assert(sessionExists(sessionID));
	assert(getUsername(sessionID) == "alice");

	cout << "Session creation test passed" << endl;

	return 0;

}