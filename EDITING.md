# Make the game yours

Start with cards.h. You can change almost all balancing numbers without touching the
networking code. Keep a backup, change one rule, rebuild with `make`, and try it.

## How the pieces talk

1. The browser loads fellowship.html, then its CSS and JavaScript.
2. JavaScript fetches `/api/fellowship/catalog`: the character/card/monster definitions.
3. Creating or joining a room gives that browser a private player token.
4. A click sends an action such as `explore`, `play`, `offer` or `fight` to Crow.
5. Crow verifies the token and the expected revision, then calls `act()` in game.h.
6. The rules engine checks whether the action is legal and updates that room.
7. Crow sends back a snapshot. The current player's hand is included; other hands are not.
8. Every 1.5 seconds each phone asks for another snapshot. A changed revision redraws the UI.

The shared room is a C++ `Room` object. A player's browser does not hold the authoritative
copy of their level, cards or equipment. Changing the browser's display won't change the
server's copy.

## Change a character

Each row in characters() initializes one Character in the order declared by its struct:

```cpp
// name, title, offense, defense, attack, chaos ability, chaos strength, chaos defense
{"Colin", "Shadow of Mirkwood", 7, 3, "Choke", "Shadow puppet", 6, 0},
```

Change 7 to 9 to increase offense. Change 6 to 8 to strengthen the chaos ability.
Chaos is currently one simple effect per character: a strength bonus for the party and
a defense bonus for the person using it. The flavorful names don't imply hidden mechanics.
For example, Amelia's projection does not copy another character's ability; its implemented
rule is +5 party strength and +2 personal defense. To add different effects, edit the
`action == "chaos"` branch in game.h and explain them in the UI.

A character's position in the vector is its numeric ID (Colin=0, Samuel=1, ... Sean=8).
Alliance combos refer to these IDs. If you reorder characters, update synergy() in game.h.
To add a tenth selectable character, add a row; the game can still cap rooms at nine.

## Change a monster

```cpp
// name, location, flavor text, strength, levels rewarded, cards rewarded, danger
{"Cave Troll", "The Mines of Moria", "They have a cave troll.", 20, 1, 3, 9},
```

Strength decides who wins a fight. Danger decides the defeat penalty for each player.
Defense >= danger means losing one level; lower defense means losing two. A failed flee
also discards a random hand card. Every monster is equally likely when exploring.
To weight encounters or scale monsters by player level, edit the `explore` branch.

## Add a card

```cpp
// name, kind, slot, rules text, strength bonus, defense bonus
{"Ranger's Sword", "gear", "weapon", "Equip: +4 strength.", 4, 0},
```

Valid existing kinds:

| Kind | What the engine does |
| --- | --- |
| gear | Equips the card in its weapon, armor or charm slot; replaces old gear |
| boost | Adds power to the fellowship for the current battle |
| sabotage | Adds power to the monster; participants cannot use it |
| level | Gives the active player one level before battle, capped below the win level |

The `text` field is displayed to players. It is **not executable**: writing “draw three
cards” there will not implement that effect. New mechanics need code in act().
For existing boost/sabotage cards, power is read automatically. Gear uses both numbers.
The level card currently always gives one level, regardless of its power field.

Adding a card automatically makes it available to draws and to the browser catalog.
Draws choose a type at random with replacement, not from a shuffled finite deck.
To make one type more common, you can repeat its row. For a true finite deck, add draw
and discard vectors to Room and update draw() to take the Room as an argument.

## Change alliances and rewards

`synergy()` in game.h adds basic teamwork and the two character combinations. For example:

```cpp
if (has(party, 8) && has(party, 3)) total += 4; // Sean + Matt
```

The party includes the active player and accepted helpers. Offers alone don't contribute.
`offer` expresses consent, `accept` commits that player, and `withdraw` cancels a pending
offer. Once accepted, helpers cannot leave that encounter. `MAX_HELPERS` limits the count.

`win()` awards levels, gold and cards. Helpers stop below the victory level so someone
must defeat a monster on their own turn to win. `lose()` handles defeat penalties.
If you change rewards or helper limits, update the explanations in fellowship.html and
fellowship.js too: several deliberately plain-language descriptions mention these numbers.

## Add a new action

Suppose you want to give players a way to earn one gold before battle. This is an example
only; allowing it repeatedly would generate infinite gold! A balanced version needs a
once-per-turn flag in Room or Player.

1. Add a named branch in act() in game.h. Validate the phase and whose turn it is with
   `require()` before mutating state.
2. Add a `button('Label', 'your_action')` in the relevant render function in fellowship.js.
3. If it needs new state, add a C++ field to Room/Player and include the appropriate field
   in snapshot() in game_routes.h. Do not publish hidden information to other players.
4. Reset once-per-turn/encounter flags in nextTurn()/resetBattle(), and on rematch if needed.
5. Extend the rules test with both allowed and forbidden uses.

The server checks every action even when the UI disables its button. Never rely on a
hidden or disabled button to enforce a rule; a client can still send its own HTTP request.

## Make it look different

The top of fellowship.css defines colors in one place:

```css
:root {
    --bg: #101c17;
    --gold: #dec087;
    --paper: #eee4cb;
}
```

The media-query sections near the end control phone and tablet layouts. Hands scroll
sideways on phones; the main content stacks vertically. No external fonts or images
need to download. Avoid shrinking touch targets to fit more cards.

fellowship.html has the permanent page structure and rules dialog. fellowship.js builds
changing content such as player lists, monster encounters and card hands. Use `esc()`
for any new text inserted into HTML; use textContent when markup isn't needed.

## A few C++ details

- `struct` groups related data: Player holds a name, level, equipment and hand.
- `std::vector<int>` is a resizable list of numbers. A hand stores card-type IDs.
- A card action uses the **index in your current hand**, not the card-type ID.
- `-1` means “none” for an empty slot, absent monster or absent winner.
- `Room& r` passes a room by reference, so changes update the stored room.
- `const Player& p` lets a function read a player without copying or modifying them.
- `inline` lets these small functions live in headers included by different source files.
- `require(condition, message)` throws if a rule is violated; Crow returns the message
  as a JSON error. Check all conditions before changing state.
- `revision` increases after a legal mutation. A stale click is rejected rather than
  accidentally using a card whose hand position changed.
- The room mutex means only one HTTP handler can read or change room state at a time.
  This simple design is sufficient for a small friends server.

## Where to start reading

Read cards.h → structs and power()/defense() in game.h → act() → win()/lose() →
fellowship.js render functions → game_routes.h last. You can change and understand the
game's rules before learning every HTTP and JSON detail.
