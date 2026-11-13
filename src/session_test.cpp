#include "session.h"
#include <cassert>
#include <iostream>

int main() {
	string sessionID = createSession("alice");

	assert(!sessionID.empty());
	assrt(sessionExists(sessionID));
	assert(getUsername(sessionID) == "alice");

	cout << "Session creation test passed" << endl;

	return 0;

}