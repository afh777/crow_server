// STANDALONE LAUNCHER: run beside your existing server without changing main.cpp.
// The same game_routes.h can instead be included in your existing main.cpp.
#include "game_routes.h"
#include <cstdlib>
#include <iostream>

int main() {
    try {
        crow::SimpleApp app;
        fellowship::registerGameRoutes(app);
        CROW_ROUTE(app, "/")([] {
            crow::response response(302);
            response.set_header("Location", "/fellowship");
            return response;
        });
        // Default 18081 leaves your existing server's port 18080 available.
        const char* setting = std::getenv("PORT");
        int port = setting ? std::stoi(setting) : 18081;
        if (port < 1 || port > 65535) throw std::runtime_error("PORT must be 1–65535");
        // Local-only by default. A tunnel can reach this without exposing Pi hardware routes.
        const char* binding = std::getenv("BIND_ADDRESS");
        app.bindaddr(binding ? binding : "127.0.0.1").port(port).run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
