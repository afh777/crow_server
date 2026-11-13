#include "database.h"
#include <sqlite3.h>
#include <iostream>

using namespace std;

void initDatabase() {
	sqlite3* db;

	int rc = sqlite3_open("users.db", &db);
	if (rc) {
		cout << "Can't open database\n";
		return;
	}

	const char* sql =
		"CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, passwordHash TEXT NOT NULL);";

	char* errorMessage = nullptr;

	rc = sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage);

	if(rc != SQLITE_OK) {
		cout << "SQL error: "
		     << endl;

		sqlite3_free(errorMessage);
	}

	sqlite3_close(db);

}

bool userExists(string username) {
	sqlite3* db;
	sqlite3_open("users.db", &db);

	string sql = "SELECT username FROM users WHERE username = ?;";

	sqlite3_stmt* stmt;

	sqlite3_prepare_v2(db, sql.c_str(),-1,&stmt,nullptr);

	sqlite3_bind_text(stmt,1,username.c_str(),-1,SQLITE_STATIC);

	bool exists = (sqlite3_step(stmt)==SQLITE_ROW);

	sqlite3_finalize(stmt);
	sqlite3_close(db);

	return exists;

}


bool addUser(
	string username,
	string passwordHash)
{
	sqlite3* db;
	sqlite3_open("users.db", &db);

	string sql = "INSERT INTO users VALUES (?, ?);";

	sqlite3_stmt* stmt;

	sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

	sqlite3_bind_text(stmt,
                          2,
                          passwordHash.c_str(),
                          -1,
                          SQLITE_STATIC);

        bool success =
                (sqlite3_step(stmt) == SQLITE_DONE);

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return success;
}

string getPasswordHash(
        string username)
{
        sqlite3* db;
        sqlite3_open("users.db", &db);

        std::string sql =
                "SELECT passwordHash "
                "FROM users "
                "WHERE username = ?;";

        sqlite3_stmt* stmt;

        sqlite3_prepare_v2(db,
                           sql.c_str(),
                           -1,
                           &stmt,
                           nullptr);

        sqlite3_bind_text(stmt,
                          1,
                          username.c_str(),
                          -1,
                          SQLITE_STATIC);

        std::string hash = "";

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
                hash =
                        reinterpret_cast<const char*>(
                                sqlite3_column_text(stmt, 0));
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);

        return hash;
}
