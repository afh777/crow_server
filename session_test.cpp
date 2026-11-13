#include "session.h"
#include <cassert>
#include <iostream>

int main() {
	string alice1 = createSession("alice");
	string alice2 = createSession("alice");
	string bob1 = createSession("bob");


	assert(aliceSession1 != aliceSession2);
	assert(aliceSession1 != bobSession);
	assert(aliceSession2 != bobSession);

	assert(getUsername(aliceSession1) == "alice");
	assert(getUsername(aliceSession2) == "alice");
	assert(getUsername(bobSession) == "bob");

	assert(sessionExists(aliceSession1));
	assert(sessionExists(aliceSession2));
	assert(sessionExists(bobSession));


	cout << "Session creation test passed" << endl;

	return 0;

}