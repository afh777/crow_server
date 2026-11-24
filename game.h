#pragma once
#include "cards.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace fellowship {
// THE RULES ENGINE. It knows nothing about web pages or HTTP.
// All player actions go through act(). The browser never decides combat results.
inline long long now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
inline void require(bool okay, const std::string& message) {
    if (!okay) throw std::runtime_error(message);
}
inline int roll(int low, int high) {
    static std::mt19937 rng(std::random_device{}());
    return std::uniform_int_distribution<int>(low, high)(rng);
}
// Room credentials come from Linux's operating-system random source.
inline std::string secret(int bytes = 24) {
    std::ifstream source("/dev/urandom", std::ios::binary);
    std::string result;
    const char* hex = "0123456789abcdef";
    for (int i = 0; i < bytes; ++i) {
        unsigned char value = 0;
        require(bool(source.read(reinterpret_cast<char*>(&value), 1)), "Random source unavailable.");
        result += hex[value >> 4]; result += hex[value & 15];
    }
    return result;
}
struct Player {
    std::string name, token;
    int character = 0, level = 1, gold = 0;
    std::vector<int> hand; // Card type IDs, never sent to other players.
    std::array<int, 3> gear{{-1, -1, -1}}; // weapon, armor, charm; -1 = empty.
    long long seen = now();
};
struct Room {
    std::string code, phase = "lobby";
    std::vector<Player> players;
    int host = 0, turn = 0, monster = -1, winner = -1, revision = 0;
    int goodBonus = 0, badBonus = 0;
    std::vector<int> helpers, offers, chaosUsed;
    std::vector<std::string> log;
    long long touched = now(), turnBegan = now(), resolveAfter = 0;
};
inline void note(Room& r, const std::string& message) {
    r.log.push_back(message);
    if (r.log.size() > 50) r.log.erase(r.log.begin());
}
inline void draw(Player& p, int count) {
    // Limit growth so a stalled battle cannot accumulate unbounded cards.
    for (int i = 0; i < count && p.hand.size() < 20; ++i)
        p.hand.push_back(roll(0, int(cards().size()) - 1));
}
inline bool has(const std::vector<int>& list, int id) {
    return std::find(list.begin(), list.end(), id) != list.end();
}
inline bool fighting(const Room& r, int id) { return id == r.turn || has(r.helpers, id); }
inline int power(const Player& p) {
    int total = p.level + characters()[p.character].offense;
    for (int id : p.gear) if (id >= 0) total += cards()[id].power;
    return total;
}
inline int defense(const Room& r, int id) {
    const auto& p = r.players[id];
    int total = characters()[p.character].defense;
    for (int item : p.gear) if (item >= 0) total += cards()[item].defense;
    if (has(r.chaosUsed, id)) total += characters()[p.character].chaosWard;
    return total;
}
inline int synergy(const Room& r) {
    // EDIT ALLIANCE COMBOS HERE. Everyone helping gives a basic teamwork bonus.
    int total = int(r.helpers.size()) * 2;
    std::vector<int> party{r.players[r.turn].character};
    for (int id : r.helpers) party.push_back(r.players[id].character);
    if (has(party, 5) && has(party, 2) && has(party, 1)) total += 5;
    if (has(party, 8) && has(party, 3)) total += 4;
    return total;
}
inline int teamPower(const Room& r) {
    int total = power(r.players[r.turn]) + r.goodBonus + synergy(r);
    for (int id : r.helpers) total += power(r.players[id]);
    return total;
}
inline int monsterPower(const Room& r) {
    return r.monster < 0 ? 0 : monsters()[r.monster].power + r.badBonus;
}
inline void resetBattle(Room& r) {
    r.monster = -1; r.goodBonus = 0; r.badBonus = 0;
    r.helpers.clear(); r.offers.clear(); r.chaosUsed.clear(); r.resolveAfter = 0;
}
inline void nextTurn(Room& r) {
    resetBattle(r);
    r.turn = (r.turn + 1) % int(r.players.size());
    r.phase = "camp"; r.turnBegan = now();
    note(r, r.players[r.turn].name + " takes the road. Equip gear, then explore.");
}
inline void finishBattle(Room& r) {
    r.phase = "spoils"; r.resolveAfter = 0;
}
inline void win(Room& r) {
    const auto& m = monsters()[r.monster];
    auto& active = r.players[r.turn];
    active.level = std::min(WIN_LEVEL, active.level + m.levels);
    active.gold += 2; draw(active, m.treasure);
    note(r, active.name + " defeats " + m.name + "! +" + std::to_string(m.levels) +
         " levels, +2 gold, +" + std::to_string(m.treasure) + " cards.");
    for (int id : r.helpers) {
        auto& helper = r.players[id];
        helper.level = std::min(WIN_LEVEL - 1, helper.level + 1);
        helper.gold += 1; draw(helper, 1);
        note(r, helper.name + " earns a helper level (maximum 9), 1 gold and 1 card.");
    }
    finishBattle(r);
    if (active.level >= WIN_LEVEL) {
        r.phase = "finished"; r.winner = r.turn;
        note(r, active.name + " reaches level " + std::to_string(WIN_LEVEL) + " and wins the fellowship's tale!");
    }
}
inline void lose(Room& r) {
    const auto& m = monsters()[r.monster];
    std::vector<int> party = r.helpers; party.push_back(r.turn);
    for (int id : party) {
        auto& p = r.players[id];
        // Defense decides how painful defeat is. Nobody dies or leaves the game.
        int loss = defense(r, id) >= m.danger ? 1 : 2;
        p.level = std::max(1, p.level - loss);
        if (!p.hand.empty()) p.hand.erase(p.hand.begin() + roll(0, int(p.hand.size()) - 1));
        note(r, p.name + " retreats: -" + std::to_string(loss) + " levels (minimum 1), and loses a random hand card if held.");
    }
    finishBattle(r);
}
inline void validateName(const std::string& name) {
    require(!name.empty() && name.size() <= 40 && name.find_first_not_of(" \t\r\n") != std::string::npos,
            "Use a name between 1 and 40 bytes.");
    for (unsigned char c : name) require(c >= 32 && c != 127, "Names cannot contain control characters.");
}
inline void addPlayer(Room& r, const std::string& name, int character) {
    require(r.phase == "lobby", "This adventure has already started.");
    require(r.players.size() < MAX_PLAYERS, "The fellowship is full.");
    validateName(name);
    require(character >= 0 && character < int(characters().size()), "Choose a valid character.");
    for (const auto& p : r.players) {
        require(p.character != character, "That character has already been chosen.");
        require(p.name != name, "That name is already in the room.");
    }
    Player p; p.name = name; p.character = character; p.token = secret();
    r.players.push_back(p); note(r, name + " joined as " + characters()[character].name + ".");
    r.revision++; r.touched = now();
}

// A single action has a name and up to two integer arguments.
// Card index means position in YOUR hand, not a freely chosen card type.
inline void act(Room& r, int who, const std::string& action, int card = -1, int target = -1) {
    require(who >= 0 && who < int(r.players.size()), "Unknown player.");
    auto& p = r.players[who];
    const bool active = who == r.turn;
    if (action == "claim_host") {
        require(now() - r.players[r.host].seen > 120, "The host must be offline for two minutes first.");
        r.host = who; note(r, p.name + " became host because the previous host disconnected.");
    } else if (action == "start") {
        require(who == r.host && r.phase == "lobby", "Only the host can start the lobby.");
        require(r.players.size() >= 2, "Gather at least two players.");
        for (auto& player : r.players) draw(player, STARTING_CARDS);
        r.phase = "camp"; r.turn = roll(0, int(r.players.size()) - 1); r.turnBegan = now();
        note(r, "The adventure begins. " + r.players[r.turn].name + " goes first.");
    } else if (action == "remove") {
        require(who == r.host && r.phase == "lobby", "Only the host can remove players before starting.");
        require(target >= 0 && target < int(r.players.size()) && target != r.host, "Choose another player.");
        note(r, r.players[target].name + " was removed from the lobby.");
        r.players.erase(r.players.begin() + target); if (r.host > target) r.host--;
    } else if (action == "skip") {
        require(who == r.host && r.phase != "lobby" && r.phase != "finished", "Only the host can skip an absent player's turn.");
        require(now() - r.players[r.turn].seen > 90 && now() - r.turnBegan > 90,
                "The active player must be offline for 90 seconds first.");
        note(r, "Host skipped the absent player's turn; no battle rewards were given."); nextTurn(r);
    } else if (action == "explore") {
        require(active && r.phase == "camp", "Explore on your turn, before battle.");
        resetBattle(r); r.monster = roll(0, int(monsters().size()) - 1); r.phase = "battle";
        r.resolveAfter = now() + 8;
        note(r, p.name + " encounters " + monsters()[r.monster].name + "!");
    } else if (action == "offer") {
        require(r.phase == "battle" && !active && !has(r.helpers, who) && !has(r.offers, who), "You cannot offer help now.");
        r.offers.push_back(who); note(r, p.name + " offers an alliance for this battle.");
    } else if (action == "withdraw") {
        require(r.phase == "battle" && has(r.offers, who), "You have no pending offer.");
        r.offers.erase(std::remove(r.offers.begin(), r.offers.end(), who), r.offers.end());
        note(r, p.name + " withdraws their offer.");
    } else if (action == "accept") {
        require(active && r.phase == "battle", "Only the active player can accept help in battle.");
        require(has(r.offers, target) && r.helpers.size() < MAX_HELPERS, "Choose an offered helper; at most two can join.");
        r.helpers.push_back(target); r.offers.erase(std::remove(r.offers.begin(), r.offers.end(), target), r.offers.end());
        r.resolveAfter = now() + 5;
        note(r, p.name + " accepts " + r.players[target].name + " into the alliance.");
    } else if (action == "chaos") {
        require(r.phase == "battle" && fighting(r, who) && !has(r.chaosUsed, who), "Use your chaos once per battle, as a participant.");
        const auto& c = characters()[p.character]; r.chaosUsed.push_back(who); r.goodBonus += c.chaosPower;
        r.resolveAfter = now() + 5;
        note(r, p.name + " uses " + c.chaos + ": +" + std::to_string(c.chaosPower) + " strength, +" + std::to_string(c.chaosWard) + " personal defense.");
    } else if (action == "play") {
        require(card >= 0 && card < int(p.hand.size()), "Choose a card in your hand.");
        const auto& c = cards()[p.hand[card]];
        if (c.kind == "gear") {
            require((active && r.phase == "camp") || (r.phase == "battle" && fighting(r, who)), "Equip before battle on your turn, or while participating in battle.");
            int slot = c.slot == "weapon" ? 0 : c.slot == "armor" ? 1 : 2;
            p.gear[slot] = p.hand[card]; // Replacing gear discards the previous item.
            if (r.phase == "battle") r.resolveAfter = now() + 5;
        } else if (c.kind == "level") {
            require(active && r.phase == "camp" && p.level < WIN_LEVEL - 1, "Play before battle on your turn, below level 9.");
            p.level++;
        } else {
            require(r.phase == "battle", "Play this during a battle.");
            if (c.kind == "boost") r.goodBonus += c.power; // Anyone can give a gift, without becoming a helper.
            else {
                require(!fighting(r, who), "Participants cannot sabotage their own battle.");
                r.badBonus += c.power;
            }
            r.resolveAfter = now() + 5; // Give opponents time to answer a new card.
        }
        note(r, p.name + " plays " + c.name + ".");
        p.hand.erase(p.hand.begin() + card);
    } else if (action == "fight") {
        require(active && r.phase == "battle", "Only the active player can resolve combat.");
        require(now() >= r.resolveAfter, "Give the other players time to answer the last play.");
        require(teamPower(r) > monsterPower(r), "You must exceed the monster's strength. Ask for help, play cards, or flee.");
        win(r);
    } else if (action == "flee") {
        require(active && r.phase == "battle", "Only the active player can flee.");
        require(now() >= r.resolveAfter, "Give players time to respond first.");
        int die = roll(1, 6);
        note(r, p.name + " rolls " + std::to_string(die) + " to flee (die + personal defense must reach 12).");
        if (die + defense(r, who) >= 12) { note(r, "The entire alliance escapes without losses."); finishBattle(r); }
        else lose(r);
    } else if (action == "buy") {
        require(active && r.phase == "camp" && p.gold >= 3 && p.hand.size() < 20, "Buy a card before battle on your turn for 3 gold.");
        p.gold -= 3; draw(p, 1); note(r, p.name + " buys a treasure card.");
    } else if (action == "discard") {
        require(r.phase != "lobby" && r.phase != "finished" && card >= 0 && card < int(p.hand.size()), "Choose a card to discard during the game.");
        p.hand.erase(p.hand.begin() + card); note(r, p.name + " discards a card.");
    } else if (action == "end") {
        require(active && r.phase == "spoils", "Finish the encounter before ending your turn.");
        require(p.hand.size() <= HAND_LIMIT, "Discard down to eight cards first."); nextTurn(r);
    } else if (action == "rematch") {
        require(who == r.host && r.phase == "finished", "Only the host can start a rematch after victory.");
        for (auto& player : r.players) {
            player.level = 1; player.gold = 0; player.hand.clear(); player.gear = {{-1, -1, -1}};
        }
        resetBattle(r); r.phase = "lobby"; r.winner = -1; r.turn = 0;
        note(r, "A new tale awaits. The host can start when everyone is ready.");
    } else throw std::runtime_error("Unknown action.");
    r.revision++; r.touched = now();
}
} // namespace fellowship
