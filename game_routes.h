#pragma once
#include "crow_all.h" // Your existing Crow header: compile with -I.. when needed.
#include "game.h"
#include <map>
#include <mutex>
#include <sstream>

namespace fellowship {
// HTTP ADAPTER. This translates JSON messages into rules-engine actions.
// One mutex protects room state even if the existing Crow app is multithreaded.
inline std::map<std::string, Room>& rooms() { static std::map<std::string, Room> all; return all; }
inline std::mutex& roomMutex() { static std::mutex mutex; return mutex; }
inline crow::response jsonResponse(crow::json::wvalue value, int status = 200) {
    crow::response response(status, value.dump());
    response.set_header("Content-Type", "application/json");
    response.set_header("Cache-Control", "no-store");
    response.set_header("X-Content-Type-Options", "nosniff");
    return response;
}
inline crow::response failure(const std::string& message, int status = 400) {
    crow::json::wvalue value; value["error"] = message; return jsonResponse(std::move(value), status);
}
inline crow::json::rvalue bodyOf(const crow::request& request) {
    require(request.body.size() <= 4096, "Request too large.");
    const auto& origin = request.get_header_value("Origin");
    const auto& host = request.get_header_value("Host");
    require(origin.empty() || origin == "https://" + host || origin == "http://" + host, "Open the game from its own web address.");
    auto body = crow::json::load(request.body);
    require(body && body.t() == crow::json::type::Object, "Expected a JSON object.");
    return body;
}
inline std::string textField(const crow::json::rvalue& body, const char* key) {
    require(body.has(key) && body[key].t() == crow::json::type::String, std::string("Missing text field: ") + key);
    return body[key].s();
}
inline int numberField(const crow::json::rvalue& body, const char* key, int fallback = -1) {
    if (!body.has(key)) return fallback;
    require(body[key].t() == crow::json::type::Number, std::string("Expected number: ") + key);
    double value = body[key].d();
    require(value >= -1 && value <= 100000000 && value == static_cast<int>(value), "Invalid whole number.");
    return static_cast<int>(value);
}
inline void cleanRooms() {
    // Abandoned rooms disappear after 12 hours. All rooms also disappear on restart.
    for (auto it = rooms().begin(); it != rooms().end();) {
        if (now() - it->second.touched > 12 * 60 * 60) it = rooms().erase(it);
        else ++it;
    }
}
inline Room& findRoom(const std::string& code) {
    auto it = rooms().find(code);
    require(it != rooms().end(), "Room not found. Check the code; a server restart clears rooms.");
    return it->second;
}
inline int playerId(const Room& r, const crow::request& request) {
    const auto& token = request.get_header_value("X-Player-Token");
    for (int i = 0; i < int(r.players.size()); ++i)
        if (!token.empty() && token == r.players[i].token) return i;
    return -1;
}
inline crow::json::wvalue catalog() {
    crow::json::wvalue out;
    out["winLevel"] = WIN_LEVEL; out["handLimit"] = HAND_LIMIT;
    out["characters"] = crow::json::wvalue::list();
    for (unsigned i = 0; i < characters().size(); ++i) {
        const auto& c = characters()[i]; auto& j = out["characters"][i];
        j["name"] = c.name; j["title"] = c.title; j["offense"] = c.offense; j["defense"] = c.defense;
        j["attack"] = c.attack; j["chaos"] = c.chaos; j["chaosPower"] = c.chaosPower; j["chaosWard"] = c.chaosWard;
    }
    out["cards"] = crow::json::wvalue::list();
    for (unsigned i = 0; i < cards().size(); ++i) {
        const auto& c = cards()[i]; auto& j = out["cards"][i];
        j["name"] = c.name; j["kind"] = c.kind; j["slot"] = c.slot; j["text"] = c.text; j["power"] = c.power;
    }
    out["monsters"] = crow::json::wvalue::list();
    for (unsigned i = 0; i < monsters().size(); ++i) {
        const auto& m = monsters()[i]; auto& j = out["monsters"][i];
        j["name"] = m.name; j["place"] = m.place; j["text"] = m.text;
        j["power"] = m.power; j["levels"] = m.levels; j["treasure"] = m.treasure; j["danger"] = m.danger;
    }
    return out;
}
inline crow::json::wvalue snapshot(const Room& r, int viewer) {
    crow::json::wvalue out;
    out["room"] = r.code; out["phase"] = r.phase; out["revision"] = r.revision;
    out["host"] = r.host; out["turn"] = r.turn; out["you"] = viewer; out["winner"] = r.winner;
    out["monster"] = r.monster; out["goodBonus"] = r.goodBonus; out["badBonus"] = r.badBonus;
    out["teamPower"] = teamPower(r); out["monsterPower"] = monsterPower(r); out["synergy"] = synergy(r);
    out["wait"] = std::max(0LL, r.resolveAfter - now());
    out["hostAbsent"] = now() - r.players[r.host].seen > 120;
    out["canSkip"] = now() - r.players[r.turn].seen > 90 && now() - r.turnBegan > 90;
    out["helpers"] = r.helpers; out["offers"] = r.offers; out["chaosUsed"] = r.chaosUsed; out["log"] = r.log;
    out["players"] = crow::json::wvalue::list();
    for (unsigned i = 0; i < r.players.size(); ++i) {
        const auto& p = r.players[i]; auto& j = out["players"][i];
        j["name"] = p.name; j["character"] = p.character; j["level"] = p.level; j["gold"] = p.gold;
        j["power"] = power(p); j["defense"] = defense(r, i); j["handCount"] = p.hand.size();
        j["online"] = now() - p.seen < 20;
        j["gear"] = crow::json::wvalue::list();
        for (unsigned slot = 0; slot < 3; ++slot) j["gear"][slot] = p.gear[slot];
        if (int(i) == viewer) j["hand"] = p.hand; // Only the owner receives their cards.
    }
    return out;
}
inline crow::response asset(const std::string& filename, const std::string& type) {
    std::ifstream file("public/" + filename, std::ios::binary);
    if (!file) return crow::response(404, "Game file missing. Run the server from the folder containing public/.");
    std::ostringstream bytes; bytes << file.rdbuf();
    crow::response response(bytes.str()); response.set_header("Content-Type", type);
    response.set_header("Cache-Control", "no-cache");
    response.set_header("X-Content-Type-Options", "nosniff");
    response.set_header("Referrer-Policy", "same-origin");
    response.set_header("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; img-src 'self' data:; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");
    return response;
}

// Call this ONCE in main(), before app.run(), to add the game to an existing server.
inline void registerGameRoutes(crow::SimpleApp& app) {
    CROW_ROUTE(app, "/fellowship")([] { return asset("fellowship.html", "text/html; charset=utf-8"); });
    CROW_ROUTE(app, "/fellowship.css")([] { return asset("fellowship.css", "text/css; charset=utf-8"); });
    CROW_ROUTE(app, "/fellowship.js")([] { return asset("fellowship.js", "application/javascript; charset=utf-8"); });
    CROW_ROUTE(app, "/api/fellowship/catalog")([] { return jsonResponse(catalog()); });

    CROW_ROUTE(app, "/api/fellowship/create").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        std::lock_guard<std::mutex> lock(roomMutex());
        try {
            auto body = bodyOf(req); cleanRooms();
            require(rooms().size() < 128, "Server is full. Try again later.");
            Room r;
            do { r.code = secret(3); std::transform(r.code.begin(), r.code.end(), r.code.begin(), ::toupper); }
            while (rooms().count(r.code));
            addPlayer(r, textField(body, "name"), numberField(body, "character"));
            auto out = snapshot(r, 0); out["token"] = r.players[0].token;
            rooms().emplace(r.code, std::move(r)); return jsonResponse(std::move(out));
        } catch (const std::exception& e) { return failure(e.what()); }
    });
    CROW_ROUTE(app, "/api/fellowship/join").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req) {
        std::lock_guard<std::mutex> lock(roomMutex());
        try {
            auto body = bodyOf(req); cleanRooms(); auto& r = findRoom(textField(body, "room"));
            addPlayer(r, textField(body, "name"), numberField(body, "character"));
            auto out = snapshot(r, int(r.players.size()) - 1); out["token"] = r.players.back().token;
            return jsonResponse(std::move(out));
        } catch (const std::exception& e) { return failure(e.what()); }
    });
    CROW_ROUTE(app, "/api/fellowship/state/<string>")
    ([](const crow::request& req, const std::string& code) {
        std::lock_guard<std::mutex> lock(roomMutex());
        try {
            auto& r = findRoom(code); int who = playerId(r, req);
            if (who < 0) return failure("Your seat is no longer available. Return to the join screen.", 401);
            r.players[who].seen = now(); r.touched = now(); return jsonResponse(snapshot(r, who));
        } catch (const std::exception& e) { return failure(e.what(), 404); }
    });
    CROW_ROUTE(app, "/api/fellowship/action/<string>").methods(crow::HTTPMethod::Post)
    ([](const crow::request& req, const std::string& code) {
        std::lock_guard<std::mutex> lock(roomMutex());
        try {
            auto body = bodyOf(req); auto& r = findRoom(code); int who = playerId(r, req);
            if (who < 0) return failure("Your seat is no longer available.", 401);
            // Reject stale clicks and duplicate submissions, rather than playing the wrong hand index.
            if (numberField(body, "revision") != r.revision) return failure("The table changed. Review the updated board and try again.", 409);
            act(r, who, textField(body, "action"), numberField(body, "card"), numberField(body, "target"));
            // Removal can shift the host's index, so authenticate again before responding.
            who = playerId(r, req); r.players[who].seen = now(); return jsonResponse(snapshot(r, who));
        } catch (const std::exception& e) { return failure(e.what()); }
    });
}
} // namespace fellowship
