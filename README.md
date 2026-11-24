# Fellowship of the Unhinged

A complete, unofficial LOTR-themed multiplayer card game for your Raspberry Pi.
2–9 players join in phone browsers. No app, account, paid service, JavaScript framework,
Node server, or external artwork is needed. The game uses your existing C++/Crow stack.

**Start here:** use the standalone option below. It runs beside your existing server on
port **18081**, keeping its login and Pi/LED routes separate. This package does not
replace your existing main.cpp or game.html. The new page is **/fellowship**.

## 1. Put the files on your Pi

Unzip this package. Copy the whole `fellowship` folder inside your existing `crow_server`
repository. The layout should be:

```
crow_server/
    crow_all.h                (already in your repository)
    main.cpp                  (your existing server stays here)
    fellowship/
        Makefile
        game_server.cpp
        cards.h
        game.h
        game_routes.h
        README.md
        EDITING.md
        public/
            fellowship.html
            fellowship.css
            fellowship.js
        tests/
```

If you transfer via Windows, VS Code Remote SSH or an SFTP client can copy the folder.
These commands run **in a terminal on the Pi**, not in Windows PowerShell.

## 2. Build and run

Install the compiler and standalone Asio headers, if needed:

```sh
sudo apt update
sudo apt install g++ make libasio-dev
```

From inside the new `fellowship` folder:

```sh
make
./fellowship_server
```

The project compiles against the `crow_all.h` already in the parent folder. Do not compile
all the repository's `.cpp` files together: that would include multiple `main()` functions.
The binary is built on your Pi, so it matches your Pi's CPU architecture.

By default it listens on `127.0.0.1:18081`. If you have a browser on the Pi, open:

```
http://127.0.0.1:18081/fellowship
```

To try phones **on the same Wi-Fi**, stop the process with Ctrl+C and run:

```sh
BIND_ADDRESS=0.0.0.0 ./fellowship_server
```

Find the Pi's LAN IP with `hostname -I`, then open `http://PI_IP:18081/fellowship` on the
phones. Replace `PI_IP` with the actual address. Use a trusted home network for plain HTTP.

**For friends outside your Wi-Fi, use the HTTPS tunnel in the next section.**
A LAN IP such as 192.168.x.x is not an internet-accessible invite link.

## 3. Make an outside-access link

A Cloudflare Quick Tunnel can give the running game a temporary public HTTPS link without
router port forwarding. First install `cloudflared` on the Pi using Cloudflare's official
instructions for your Pi OS and CPU:

https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/downloads/

With the game running on port 18081, open a **second Pi terminal** and run:

```sh
cloudflared tunnel --url http://localhost:18081
```

Open the printed `https://...trycloudflare.com/fellowship` address yourself first. Create
a room, then press **Copy invite link**. Send that complete link to your friends; it
includes the room code. Each friend chooses a different character and joins.

Both the game server and cloudflared must stay running. The link changes when the Quick
Tunnel restarts. Quick Tunnels are intended for testing and development, with no uptime
guarantee. For a permanent hostname, follow Cloudflare's named-tunnel setup:

https://developers.cloudflare.com/cloudflare-one/networks/connectors/cloudflare-tunnel/

Official Quick Tunnel reference:
https://developers.cloudflare.com/tunnel/get-started/quick-tunnels/

Point the tunnel at the **game's port 18081**, rather than your existing server's 18080.
This exposes the card game independently of the existing hardware and authentication routes.
No tunnel or Raspberry Pi deployment has been performed by this package.

## 4. Play

1. The first player creates a room and shares the invite link.
2. At least one friend joins. Each character can be chosen only once per room.
3. The host starts. Each player gets four random cards; a random player goes first.
4. On your turn, equip gear, play blessings and optionally buy cards. Then explore.
5. A monster appears. Ask for help; up to two volunteers can join your battle.
6. Anyone can play aid cards. Outsiders can strengthen the monster with shadow cards.
7. Participants can equip gear and use their chaos ability once per encounter.
8. After the response window, claim victory if your strength strictly exceeds the monster,
   or attempt to flee. Losing flight costs levels and a random hand card, never elimination.
9. Defeated monsters reward the active player with the listed levels/cards and two gold.
   Each helper gets one level (capped at nine), one card and one gold.
10. Discard down to eight cards and end your turn. First to level ten through a monster
    victory on their own turn wins. The host can then start a rematch.

**Alliances are voluntary, last one encounter, and lock in once accepted.** Helpers cannot
withdraw after acceptance or sabotage their own battle. They can decline to use cards.
Aid from a non-helper is a gift and earns no automatic reward. Players can discuss deals
in person or on their own call; there is no built-in chat or item trading.

Strength = level + offense + equipment + chaos + cards + allies + teamwork.
Defense reduces defeat penalties and helps the active player's flee roll; it is not
subtracted from monster strength. Flee succeeds if d6 + active player's defense >= 12.
A successful flee saves all participants. Failed flight penalizes each participant: one
level if their defense meets the monster's danger, otherwise two, plus a random hand card.

Special combos: **Amelia + Kaitlyn + Samuel** add five strength; **Sean + Matt** add four.
Each accepted helper also adds two normal teamwork strength.

Draws are random with replacement. Gear has weapon/armor/charm slots. Equipping a new item
discards the old item in that slot. Hands can temporarily exceed eight but hold at most
twenty; extra cards above twenty are lost. The level-10 winning rule and helper caps
are controlled by WIN_LEVEL in cards.h; update explanatory UI text if changing the goal.

The initial battle has an eight-second response window. Playing a combat-affecting card,
using chaos, equipping in battle or accepting an ally gives everyone five more seconds
from that action. Victory is never automatic: the active player chooses when to resolve.

## 5. Reconnect and host controls

- Reload the same browser tab to reconnect to your private seat. Its token is stored in
  sessionStorage, never in the invite URL. Switching to a different phone/browser does
  not recover that seat. Browser tab-restore behavior varies; don't rely on closing tabs.
- **Switch table** preserves the old seat token in this tab. Reopen that room's invite
  here to return. Seats are reserved, not removed by navigating away.
- The host can remove people before starting, useful for abandoned lobby seats.
- If the active player stops contacting the server for 90 seconds, the host sees a skip
  button. Skipping gives no battle rewards and advances the turn.
- If the host is offline for two minutes, another player can claim host.
- Keep the page visible when taking your turn. Phone browsers may suspend background tabs;
  returning to the page triggers a refresh.
- To test several players yourself, use separate browser tabs opened normally or separate
  browsers/private windows. Duplicating a tab can copy its existing seat token.

## 6. Add the game to your existing main.cpp instead (optional)

The standalone launcher is the recommended first setup. If you later want one server:

1. Copy the three new `public/fellowship.*` files into your existing root `public/` folder.
2. Keep `fellowship/cards.h`, `game.h` and `game_routes.h` together.
3. Add near the top of your existing `main.cpp`:

```cpp
#include "fellowship/game_routes.h"
```

4. Immediately after your existing `crow::SimpleApp app;`, add:

```cpp
fellowship::registerGameRoutes(app);
```

5. Rebuild your existing server using its normal command. Add `-I.` if needed so the
   nested header can find the root `crow_all.h`. Run from your repository root, where
   `public/` now contains the new assets. Visit `/fellowship` on its normal port.

Do not also compile `game_server.cpp` in that build: it has the standalone main().
Do not register the routes twice. Your old `/game` page can link to `/fellowship`.
If sharing the existing server publicly, review its other routes first; the standalone
option avoids including those routes in the game's public service.

## 7. Change the game

Read **EDITING.md** for a guided explanation and examples.

| File | What to change |
| --- | --- |
| cards.h | Character stats, chaos powers, cards, monsters and constants |
| game.h | Turns, alliances, combat, rewards, flight and victory |
| game_routes.h | API routes, private snapshots and room storage |
| game_server.cpp | Listening port and bind address |
| public/fellowship.html | Page structure and the player rules dialog |
| public/fellowship.css | Colors, spacing, responsive layout |
| public/fellowship.js | Display, buttons, reconnecting and polling |

After changing C++ headers: run `make` and restart the game server. Restarting clears
active rooms. After editing HTML/CSS/JS: refresh the browser; no C++ rebuild is needed.

## 8. Checks and limitations

Run from inside fellowship/:

```sh
make test
python3 tests/http_test.py
```

The C++ tests cover permissions, rewards, chaos, gear, alliance consent, combos, level
caps, flight, response windows, shop, rematches, discard rules and host recovery.
They also simulate 80 complete matches with 2–9 players and check that each reaches a winner.
The HTTP tests start the actual binary on a separate localhost port and check assets,
validation, private hands, hidden credentials, concurrent duplicate submissions,
reconnecting and illegal actions. Python 3 is only needed for these optional tests.

Optional DOM checks (Node and linkedom are only development tools, never game requirements):

```sh
npm install --no-save linkedom
node tests/ui_smoke.cjs
```

Validation performed for this build: C++ compilation against your repository's Crow
header, rules tests, live HTTP tests, and DOM-based client interaction tests. A visual
phone-browser check could not run in the build environment because Chromium could not
start. Please try the page on your actual phones; responsive layouts are included for
phone, tablet and desktop widths.

Rooms live **in memory**: restarting the process clears them. Abandoned rooms expire
after 12 hours, with a 128-room server cap. There is no database, account recovery,
spectator mode, matchmaking, chat, bots or saved campaign. Room codes are invitations,
not passwords. Keep your seat token private. This is a friends-scale game, not a hardened
public matchmaking platform. All rules and hand visibility are enforced server-side;
the browser cannot simply award itself cards or levels.

This is an unofficial, noncommercial fan project. No official artwork, soundtrack or
commercial game's card text is included. Character titles and mechanically defined chaos
powers are new additions; the PDF's original names, offense/defense and ability names
have been preserved apart from punctuation/capitalization.
