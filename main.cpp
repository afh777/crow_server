#include "crow_all.h"
#include "functions.h"
#include "auth.h"
#include "database.h"
#include "session.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

bool ledState = false;

int main() {

	initDatabase();
	crow::SimpleApp app;

//main page
	CROW_ROUTE(app, "/")([](){
		
		std::ifstream file("public/index.html");
		
		if (!file) {
			return crow::response(404);
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		return crow::response(buffer.str());
	});


//login page
	CROW_ROUTE(app, "/login")([]() {
		std::ifstream file("public/login.html");
		if (!file) {
			return crow::response(404);
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		
		return crow::response(buffer.str());
	});

//registering page
	CROW_ROUTE(app, "/register")([]() {
		std::ifstream file("public/register.html");
		if(!file) {
			return crow::response(404);
		}
	
		std::stringstream buffer;
		buffer << file.rdbuf();

		return crow::response(buffer.str());
	});


//LED page
	CROW_ROUTE(app, "/page2")([](){

		std::ifstream file("public/page2.html");
		
		if (!file) {
			return crow::response(404);
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		return crow::response(buffer.str());
	});

//serving authentication script
	CROW_ROUTE(app, "/auth.js")([]() {
		std::ifstream file("public/auth.js");
		if (!file) {
			return crow::response(404);
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		crow::response res(buffer.str());

		res.set_header("Contest-Type",
			"application/javascript");

		return res;
	});


//register button route
	CROW_ROUTE(app, "/api/register")
		.methods("POST"_method)
		([](const crow::request& req){

		auto body = crow::json::load(req.body);
		if (!body) {
			return crow::response(400);
		}

		std::string username = body["username"].s();
		std::string password = body["password"].s();

		if (registerUser(username, password)) {
			return crow::response("User created");
		}

		return crow::response(400, "Failed");

	});


// login button route
	CROW_ROUTE(app, "/api/login")
		.methods("POST"_method)
		([](const crow::request& req) {

		auto body = crow::json::load(req.body);

		if (!body) {
			return crow::response(400);
		}

		std::string username = body["username"].s();
		std::string password = body["password"].s();

		if (loginUser(username, password)) {
												
			std::string sessionID = createSession(username);

			crow::json::wvalue result;
			result["message"] = "Login successful";
			result["sessionID"] = sessionID;

			return crow::response(result);
		}

		return crow::response(401, "Incorrect password");
	});

//color picker
	CROW_ROUTE(app, "/api/color/<string>") ([](std::string color) {
		//for color picker
		setColor(color);
		return "Ok";
	});


//call LED function from functions.h
	CROW_ROUTE(app, "/api/led/on") ([](){
		turnLightsOn();
		return "LED ON";
	});


//style guide
	CROW_ROUTE(app, "/style.css") ([](){
		std::ifstream file("public/style.css");
		if(!file) {
			return crow::response(404);
		}

		std::stringstream buffer;
		buffer << file.rdbuf();

		crow::response res(buffer.str());

		res.set_header("Content-Type", "text/css");

		return res;
	});


//status page
	CROW_ROUTE(app, "/api/status") ([](){
		crow::json::wvalue x;
                x["online"] = true;
                x["temp"] = 72;
                return x;
	});


	app.port(18080).run();
	return 0;
}
