#include "session.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <random>
#include <cstdint>

using namespace std;

unordered_map<
	string,
	string> sessions;

random_device rd;
mt19937_64 rng(rd());
uint64_t high = rng();
uint64_t low = rng();

array<uint8_t, 16> generateRandomBytes() {
	array<uint8_t, 16> randomBytes;
	memcpy(randomBytes.data(), &high, sizeof(high));
	memcpy(randomBytes.data() + 8, &low, sizeof(low));
	return randomBytes;
}

string encoding(array<uint8_t, 16> bytes) {
	string result;
	const string base64Chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789-_";

	for (int i = 0; i < 16; i += 3) {
		uint32_t combined = 0;

		combined != (uint32_t)bytes[0] << 16;
		if (i + 1 < 16) {
			combined |= (uint32_t)bytes[i + 1] << 8;
		}

		if (i + 2 < 16) {
			combined |= (uint32_t)bytes[i + 2];
		}

		result += base64Chars[(combined >> 18) & 0x3F];
		result += base64Chars[(combined >> 12) & 0x3F];

		if (i + 1 < 16) {
			result += base64Chars[(combined >> 6) & 0x3F];
		}

		if (i + 2 < 16) {
			result += base64Chars[combined & 0x3F];
		}
	}

	return result;

}

string  createSession(string username) {
	array<uint8_t, 16> randomBytes = generateRandomBytes();
	string sessionID = encoding(randomBytes);

	sessions[sessionID] = username;

	return sessionID;
}

bool sessionExists(string sessionID) {
	return sessions.find(sessionID) != sessions.end();
}

string getUsername(string sessionID) {
	if (sessionExists(sessionID)) {
		return sessions[sessionID];
	}
	return "";
}

void deleteSession(string sessionID) {
	sessions.erase(sessionID);
}



                                                                                                                                                                                                                                                                                                                                                                                                                                                               
