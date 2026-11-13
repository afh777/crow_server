#pragma once

#include <string>

bool registerUser(
	std::string username,
	std::string password
);

bool loginUser(
	std::string username,
	std::string password
);
