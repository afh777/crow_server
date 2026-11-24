#pragma once
#include <string>
#include <vector>

namespace fellowship {
// EDIT THIS FILE to change characters, items, monsters, or game balance.
// These are original house rules inspired by dungeon card games.
constexpr int WIN_LEVEL = 10;
constexpr int MAX_PLAYERS = 9;
constexpr int MAX_HELPERS = 2;
constexpr int HAND_LIMIT = 8;
constexpr int STARTING_CARDS = 4;

struct Character {
    std::string name, title;
    int offense, defense;
    std::string attack, chaos;
    int chaosPower, chaosWard; // Extra battle strength and protection from defeat.
};
inline const std::vector<Character>& characters() {
    static const std::vector<Character> data = {
        {"Colin", "Shadow of Mirkwood", 7, 3, "Choke", "Shadow puppet", 6, 0},
        {"Samuel", "Sage of Rivendell", 3, 7, "Obscure philosophy", "Tape gun", 3, 4},
        {"Kaitlyn", "Blades of Lothlorien", 6, 4, "Knives", "Intense stare and hair flip", 5, 1},
        {"Matt", "Guardian of Gondor", 2, 8, "Crab walk", "Mt Vernon Boys: call in backup", 3, 5},
        {"Will", "Wanderer of the Shire", 4, 6, "Rubber bands", "Dance off", 4, 3},
        {"Amelia", "Strategist of the White Council", 5, 5, "Ten year plan to ruin someone's life", "Project personalities into Kaitlyn and/or Samuel", 5, 2},
        {"Elliot", "Cartographer of the North", 1, 9, "Schemer (subtle knife)", "Geography", 2, 6},
        {"Catherine", "Captain of Rohan", 8, 2, "Backhand slap to the head", "Football knowledge", 7, 0},
        {"Sean", "Lorekeeper of Erebor", 3, 7, "Crab walk", "Warhammer lore", 4, 4}
    };
    return data;
}

struct Card {
    std::string name, kind, slot, text;
    int power, defense;
};
// kind = gear, boost, sabotage, or level. Gear has one of three slots.
// Cards are drawn with replacement: there is no finite deck to run out.
inline const std::vector<Card>& cards() {
    static const std::vector<Card> data = {
        {"Blade of Gondor", "gear", "weapon", "Equip: +3 strength.", 3, 0},
        {"Elven Bow", "gear", "weapon", "Equip: +4 strength.", 4, 0},
        {"Dwarven Axe", "gear", "weapon", "Equip: +5 strength.", 5, 0},
        {"Mithril Shirt", "gear", "armor", "Equip: +2 strength, +4 defense.", 2, 4},
        {"Ranger Cloak", "gear", "armor", "Equip: +1 strength, +3 defense.", 1, 3},
        {"Shield of Rohan", "gear", "armor", "Equip: +2 strength, +2 defense.", 2, 2},
        {"Light of Earendil", "gear", "charm", "Equip: +3 strength, +1 defense.", 3, 1},
        {"Hobbit Luck", "gear", "charm", "Equip: +1 strength, +3 defense.", 1, 3},
        {"Dwarven Rune", "gear", "charm", "Equip: +2 strength, +2 defense.", 2, 2},
        {"Second Breakfast", "boost", "", "Give the fellowship +4 this battle.", 4, 0},
        {"Gandalf Arrives", "boost", "", "Give the fellowship +8 this battle.", 8, 0},
        {"Horn of Gondor", "boost", "", "Give the fellowship +5 this battle.", 5, 0},
        {"Eagles!", "boost", "", "Give the fellowship +6 this battle.", 6, 0},
        {"Lembas Bread", "boost", "", "Give the fellowship +3 this battle.", 3, 0},
        {"Eye of Sauron", "sabotage", "", "Give the monster +5 this battle.", 5, 0},
        {"Orc Reinforcements", "sabotage", "", "Give the monster +4 this battle.", 4, 0},
        {"The Ring's Temptation", "sabotage", "", "Give the monster +6 this battle.", 6, 0},
        {"Council's Blessing", "level", "", "Gain a level, up to level 9. Play on your turn before battle.", 1, 0}
    };
    return data;
}
struct Monster { std::string name, place, text; int power, levels, treasure, danger; };
inline const std::vector<Monster>& monsters() {
    static const std::vector<Monster> data = {
        {"Hungry Barrow-wight", "The Barrow-downs", "Something beneath the hill has woken.", 8, 1, 2, 5},
        {"Goblin Scavengers", "The Misty Mountains", "They want your boots. And your lunch.", 10, 1, 2, 6},
        {"Warg Pack", "The Wilds", "The howling is getting closer.", 13, 1, 2, 7},
        {"Uruk-hai Captain", "Isengard", "The White Hand offers no mercy.", 16, 1, 3, 8},
        {"Cave Troll", "The Mines of Moria", "They have a cave troll.", 20, 1, 3, 9},
        {"Nazgul Rider", "The King's Road", "Stay off the road.", 24, 2, 3, 10},
        {"Shelob", "Cirith Ungol", "A light would be useful about now.", 28, 2, 4, 11},
        {"The Balrog", "The Bridge of Khazad-dum", "You might want a fellowship for this.", 34, 2, 4, 12},
        {"The Witch-king", "The Pelennor Fields", "The shadow of Angmar returns.", 38, 2, 5, 13}
    };
    return data;
}
} // namespace fellowship
