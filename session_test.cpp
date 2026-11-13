#include "session.h"
#include <cassert>
#include <iostream>

int main() {
	string aliceSession1 = createSession("alice");
	string aliceSession2 = createSession("alice");
	string bobSession = createSession("bob");
	string invalidsession = "invalidsession";

	assert(!sessionExists(invalidsession));
	assert(getUsername(invalidsession) == "");
	cout << "invalid session passed" << endl;


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

	deleteSession(aliceSession1);

	assert(!sessionExists(aliceSession1));
	assert(getUsername(aliceSession1) == "");

	cout << "session deletion test passed" << endl;

	deleteSession("fake_session_id");

	assert(!sessionExists("fake_session_id"));

	cout << "Nonexistent deletion test passed!" << endl;

	return 0;

}