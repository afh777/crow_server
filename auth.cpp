#include "auth.h"
#include "database.h"
#include <iostream>

bool registerUser(
        std::string username,
        std::string password)
{
        std::cout << "Registering: "
                  << username
                  << std::endl;

        if (userExists(username))
        {
                std::cout << "User already exists"
                          << std::endl;

                return false;
        }

        bool success =
                addUser(username, password);

        std::cout << "addUser returned: "
                  << success
                  << std::endl;

        return success;
}
bool loginUser(
	std::string username,
	std::string password)
{
        return getPasswordHash(username)
                == password;
}
