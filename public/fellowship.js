/* FELLOWSHIP CLIENT
 * This file displays the table and sends requests. It never awards levels or draws cards.
 * cards.h = game data; game.h = rules; game_routes.h = server communication.
 * We poll once every 1.5 seconds: simpler to understand than WebSockets for a turn-based game.
 */
"use strict";
const $ = (id) => document.getElementById(id);
let catalog,
  state = null,
  token = "",
  room = "",
  busy = false,
  pollTimer;
let noticeTimer,
  lastRendered = -1;

// Escape ALL values inserted into HTML, including player names.
function esc(value) {
  return String(value).replace(
    /[&<>"']/g,
    (c) =>
      ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[
        c
      ],
  );
}
function notify(message, error = false) {
  $("notice").textContent = message;
  $("notice").classList.toggle("error", error);
  $("notice").hidden = false;
  clearTimeout(noticeTimer);
  noticeTimer = setTimeout(
    () => {
      $("notice").hidden = true;
    },
    error ? 8000 : 4000,
  );
}
// Storage failure (for example private-browser restrictions) should not block play.
function remember() {
  try {
    sessionStorage.setItem("fellowship-seat-" + room, token);
  } catch {
    notify("Seat could not be saved. Keep this page open to stay connected.");
  }
}
function savedSeat(code) {
  try {
    return sessionStorage.getItem("fellowship-seat-" + code) || "";
  } catch {
    return "";
  }
}
async function request(path, body, auth = true) {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 12000);
  try {
    const headers = {};
    if (body !== undefined) headers["Content-Type"] = "application/json";
    if (auth && token) headers["X-Player-Token"] = token;
    const response = await fetch("/api/fellowship/" + path, {
      method: body === undefined ? "GET" : "POST",
      headers,
      body: body === undefined ? undefined : JSON.stringify(body),
      signal: controller.signal,
      cache: "no-store",
    });
    const text = await response.text();
    let data;
    try {
      data = JSON.parse(text);
    } catch {
      throw new Error(
        "The server returned an unreadable response. Check that the game server is running.",
      );
    }
    if (!response.ok) {
      const error = new Error(data.error || "Request failed.");
      error.status = response.status;
      throw error;
    }
    $("connection").textContent = "The beacons are lit";
    return data;
  } catch (error) {
    if (!error.status) {
      $("connection").textContent = "Connection interrupted";
      if (error.name === "AbortError")
        throw new Error("The server took too long. Reconnecting…");
    }
    throw error;
  } finally {
    clearTimeout(timeout);
  }
}

// CHARACTER PREVIEW: all the stats come from the server catalog, not a duplicate list.
function preview() {
  const c = catalog.characters[Number($("character").value)];
  $("character-preview").innerHTML =
    `<h3>${esc(c.name)}</h3><p class="title">${esc(c.title)}</p>
    <div class="stats"><span><strong>${c.offense}</strong>OFFENSE</span><span><strong>${c.defense}</strong>DEFENSE</span></div>
    <p><strong>Attack:</strong> ${esc(c.attack)}</p><p><strong>Chaos:</strong> ${esc(c.chaos)}</p>
    <p class="gold">Once per battle: +${c.chaosPower} strength, +${c.chaosWard} personal defense.</p>`;
}
function showEntry() {
  clearTimeout(pollTimer);
  state = null;
  token = "";
  room = "";
  lastRendered = -1;
  $("table").hidden = true;
  $("loading").hidden = true;
  $("entry").hidden = false;
  history.replaceState(null, "", location.pathname);
}
function setTable(data) {
  state = data;
  room = data.room;
  if (data.token) {
    token = data.token;
    remember();
  }
  history.replaceState(
    null,
    "",
    location.pathname + "?room=" + encodeURIComponent(room),
  );
  $("entry").hidden = true;
  $("loading").hidden = true;
  $("table").hidden = false;
  render();
}
async function join(event) {
  event.preventDefault();
  if (busy) return;
  const mode = event.submitter?.value || "create";
  const code = $("room-code").value.trim().toUpperCase();
  const name = $("name").value.trim();
  if (!name) return notify("Enter your name first.", true);
  if (new TextEncoder().encode(name).length > 40)
    return notify("Please use a shorter name (40 bytes maximum).", true);
  if (mode === "join" && !/^[0-9A-F]{6}$/.test(code))
    return notify("Enter the six-character room code from your friend.", true);
  busy = true;
  document.querySelectorAll("#join-form button").forEach((b) => {
    b.disabled = true;
  });
  try {
    // If this tab already owns a seat in the chosen room, restore it instead of duplicating it.
    const existing = mode === "join" ? savedSeat(code) : "";
    if (existing) {
      room = code;
      token = existing;
      try {
        setTable(await request("state/" + room));
        return;
      } catch (error) {
        if (![401, 404].includes(error.status)) throw error;
        token = "";
      }
    }
    const data = await request(
      mode,
      { name, character: Number($("character").value), room: code },
      false,
    );
    setTable(data);
  } catch (error) {
    notify(error.message, true);
  } finally {
    busy = false;
    document.querySelectorAll("#join-form button").forEach((b) => {
      b.disabled = false;
    });
    if (state) {
      render(true);
      schedulePoll();
    }
  }
}

// ACTION BUTTONS: same event handler serves every dynamically created card button.
function button(label, action, options = {}) {
  const {
    card,
    target,
    disabled = false,
    style = "secondary",
    id = "",
  } = options;
  return `<button type="button" class="${esc(style)}" data-action="${esc(action)}"${card === undefined ? "" : ` data-card="${card}"`}${target === undefined ? "" : ` data-target="${target}"`}${disabled || busy ? " disabled" : ""}${id ? ` id="${esc(id)}"` : ""}>${esc(label)}</button>`;
}
function participant(id) {
  return id === state.turn || state.helpers.includes(id);
}
function myTurn() {
  return state.you === state.turn;
}
function canPlay(card) {
  const active = myTurn(),
    battle = state.phase === "battle";
  if (card.kind === "gear")
    return (
      (active && state.phase === "camp") || (battle && participant(state.you))
    );
  if (card.kind === "level")
    return (
      active &&
      state.phase === "camp" &&
      state.players[state.you].level < catalog.winLevel - 1
    );
  if (card.kind === "sabotage") return battle && !participant(state.you);
  return battle;
}
function renderPlayers() {
  $("player-count").textContent = state.players.length + " / 9";
  $("players").innerHTML = state.players
    .map((p, i) => {
      const c = catalog.characters[p.character];
      let badge = i === state.host ? "HOST" : "";
      if (state.phase !== "lobby" && i === state.turn) badge = "ON THE ROAD";
      if (state.helpers.includes(i)) badge = "ALLY";
      const gear = p.gear
        .filter((id) => id >= 0)
        .map((id) => catalog.cards[id].name)
        .join(" · ");
      return `<article class="player ${i === state.turn && state.phase !== "lobby" ? "active" : ""}">
      <div class="player-heading"><span class="player-name">${esc(p.name)}${i === state.you ? " (you)" : ""}</span>${badge ? `<span class="badge">${badge}</span>` : ""}</div>
      <p class="player-meta">${esc(c.name)} · <span data-online="${i}">${p.online ? "Connected" : "Away"}</span></p>
      <p class="player-meta">Level <strong>${p.level}</strong> · Strength ${p.power} · Defense ${p.defense}</p>
      <progress max="${catalog.winLevel}" value="${p.level}" aria-label="${esc(p.name)}'s level"></progress>
      <p class="player-meta">${p.handCount} cards · <span class="gold">${p.gold} gold</span></p>
      ${gear ? `<p class="player-gear">${esc(gear)}</p>` : ""}
      ${state.phase === "lobby" && state.you === state.host && i !== state.host ? button("Remove", "remove", { target: i, style: "quiet small" }) : ""}
      </article>`;
    })
    .join("");
}
function renderEncounter() {
  const p = state.players[state.turn],
    active = myTurn();
  let html = "";
  if (state.phase === "lobby") {
    html = `<p class="eyebrow">CHAPTER ZERO · THE GATHERING</p><div class="scene-symbol" aria-hidden="true">♧</div><h2>A fellowship is forming.</h2><p class="description">Share the invite link, choose your companions, and prepare to take the road. Characters are unique to each room.</p>
      <div class="waiting">${state.players.length} of 9 companions have answered the call.</div>
      <div class="actions">${state.you === state.host ? button("Begin the adventure →", "start", { style: "primary", disabled: state.players.length < 2 }) : '<p class="muted">Waiting for the host to begin.</p>'}</div>`;
  } else if (state.phase === "finished") {
    const winner = state.players[state.winner];
    html = `<p class="eyebrow">THE FINAL CHAPTER</p><div class="scene-symbol" aria-hidden="true">♕</div><h2>${esc(winner.name)} takes the crown.</h2><p class="description">Level ${winner.level}. A legend of Middle-earth. And probably no longer trusted by their friends.</p><div class="actions">${state.you === state.host ? button("Gather for a rematch", "rematch", { style: "primary" }) : '<p class="muted">The host can gather everyone for a rematch.</p>'}</div>`;
  } else if (state.phase === "camp") {
    html = `<p class="eyebrow">MAKE CAMP · ${active ? "YOUR TURN" : esc(p.name) + "'S TURN"}</p><div class="scene-symbol" aria-hidden="true">⌁</div><h2>${active ? "The road is calling." : esc(p.name) + " is preparing."}</h2><p class="description">Equip your best gear, use a blessing, or buy a treasure card. When you're ready, find out what waits beyond the firelight.</p><div class="actions">${active ? button("Explore Middle-earth →", "explore", { style: "primary" }) + button("Buy a card · 3 gold", "buy", { disabled: p.gold < 3 || p.handCount >= 20 }) : '<p class="waiting">Another companion takes the lead. Your chance to help—or meddle—is coming.</p>'}</div>`;
  } else {
    const m = catalog.monsters[state.monster],
      battle = state.phase === "battle";
    html = `<p class="eyebrow">${esc(m.place)} · ${battle ? "ENCOUNTER" : "AFTERMATH"}</p><h2>${esc(m.name)}</h2><p class="description">${esc(m.text)}</p>
      <div class="battle-score"><div class="score"><strong>${state.teamPower}</strong>FELLOWSHIP</div><span class="versus">vs.</span><div class="score enemy"><strong>${state.monsterPower}</strong>SHADOW</div></div>
      <p class="battle-detail">Teamwork +${state.synergy} · Played boosts +${state.goodBonus} · Monster boosts +${state.badBonus}</p>
      <p class="battle-detail">Victory: ${m.levels} level${m.levels > 1 ? "s" : ""}, ${m.treasure} cards, 2 gold · Danger ${m.danger}</p>
      <p class="battle-result">${battle ? (state.teamPower > state.monsterPower ? "The fellowship has the upper hand." : "The shadows are stronger. Find help, play a card, or flee.") : "The encounter is over. Gather your spoils and prepare for the next turn."}</p>`;
    if (state.helpers.length)
      html += `<p class="description">Fighting together: ${[state.turn, ...state.helpers].map((id) => esc(state.players[id].name)).join(" + ")}</p>`;
    if (battle) {
      html += '<div class="actions">';
      if (!active && !participant(state.you))
        html += state.offers.includes(state.you)
          ? button("Withdraw help offer", "withdraw", { style: "quiet" })
          : button("Offer an alliance", "offer", { style: "primary" });
      if (participant(state.you))
        html += button("Use chaos ability", "chaos", {
          disabled: state.chaosUsed.includes(state.you),
        });
      if (active)
        html +=
          button("Claim victory", "fight", {
            style: "primary",
            id: "fight-button",
            disabled: state.teamPower <= state.monsterPower || state.wait > 0,
          }) +
          button("Attempt to flee", "flee", {
            style: "danger",
            id: "flee-button",
            disabled: state.wait > 0,
          });
      html += '</div><p id="response-window" class="battle-detail"></p>';
      if (active)
        html += state.offers
          .map(
            (id) =>
              `<div class="offer"><span><strong>${esc(state.players[id].name)}</strong> offers +${state.players[id].power} strength.<br>They earn 1 level, 1 card, 1 gold on victory.</span>${button("Accept", "accept", { target: id, disabled: state.helpers.length >= 2, style: "secondary small" })}</div>`,
          )
          .join("");
      else if (state.offers.includes(state.you))
        html +=
          '<p class="waiting">Offer sent. The active player decides whether to accept.</p>';
    } else if (active)
      html += `<div class="actions">${button("End turn →", "end", { style: "primary", disabled: p.handCount > catalog.handLimit })}</div>${p.handCount > catalog.handLimit ? `<p class="battle-result">Discard ${p.handCount - catalog.handLimit} card(s) to end your turn.</p>` : ""}`;
    else
      html += `<p class="waiting">Waiting for ${esc(p.name)} to end their turn.</p>`;
  }
  html += '<div class="actions" id="host-tools"></div>';
  $("encounter").innerHTML = html;
}
function renderHand() {
  const me = state.players[state.you],
    hand = me.hand || [];
  $("hand-count").textContent = hand.length + " / " + catalog.handLimit;
  $("hand-tip").textContent =
    state.phase === "lobby"
      ? "Your starting cards arrive when the host begins."
      : "Only you can see these cards. Equipment replaces the item in the same slot.";
  $("hand").innerHTML = hand.length
    ? hand
        .map((id, index) => {
          const c = catalog.cards[id];
          const playable = canPlay(c),
            discardable = !["lobby", "finished"].includes(state.phase);
          return `<article class="card ${esc(c.kind)}"><span class="card-type">${esc(c.kind === "sabotage" ? "SHADOW" : c.kind === "boost" ? "AID" : c.kind === "level" ? "BLESSING" : c.slot)}</span><h3>${esc(c.name)}</h3><p>${esc(c.text)}</p><div class="actions">${button(c.kind === "gear" ? "Equip" : "Play", "play", { card: index, disabled: !playable, style: "primary" })}${button("Discard", "discard", { card: index, disabled: !discardable, style: "quiet" })}</div></article>`;
        })
        .join("")
    : '<p class="muted">No cards in your hand yet.</p>';
}
function renderCharacter() {
  const me = state.players[state.you],
    c = catalog.characters[me.character];
  $("your-character").innerHTML =
    `<p class="eyebrow">YOUR COMPANION</p><h2>${esc(c.name)}</h2><p class="character-note">${esc(c.title)}</p><div class="stats"><span><strong>${me.power}</strong>STRENGTH</span><span><strong>${me.defense}</strong>DEFENSE</span><span><strong>${me.gold}</strong>GOLD</span></div><p class="character-note"><strong>Attack:</strong> ${esc(c.attack)}</p><p class="character-note"><strong>${esc(c.chaos)}</strong><br>+${c.chaosPower} strength, +${c.chaosWard} defense. Once per battle as a participant.</p><ul class="gear-list">${me.gear.map((id, i) => `<li><span>${["Weapon", "Armor", "Charm"][i]}</span>${id < 0 ? "Empty slot" : esc(catalog.cards[id].name)}</li>`).join("")}</ul>`;
}
function renderLive() {
  // Time and presence change without changing the game revision.
  const wait = $("response-window");
  if (wait)
    wait.textContent =
      state.wait > 0
        ? `Response window: ${state.wait}s. Other players can answer with cards.`
        : "Response window open. The active player may resolve the encounter.";
  if ($("fight-button"))
    $("fight-button").disabled =
      busy || state.wait > 0 || state.teamPower <= state.monsterPower;
  if ($("flee-button")) $("flee-button").disabled = busy || state.wait > 0;
  document.querySelectorAll("[data-online]").forEach((el) => {
    const online = state.players[Number(el.dataset.online)].online;
    el.textContent = online ? "Connected" : "Away";
    el.classList.toggle("offline", !online);
  });
  const tools = $("host-tools");
  const contents =
    (state.hostAbsent && state.you !== state.host
      ? button("Claim absent host seat", "claim_host", { style: "quiet small" })
      : "") +
    (state.canSkip &&
    state.you === state.host &&
    !["lobby", "finished"].includes(state.phase)
      ? button("Skip absent player", "skip", { style: "quiet small" })
      : "");
  if (tools && tools.innerHTML !== contents) tools.innerHTML = contents;
}
function render(force = false) {
  if (!state) return;
  if (force || lastRendered !== state.revision) {
    lastRendered = state.revision;
    $("room-title").textContent =
      state.phase === "lobby"
        ? "The gathering"
        : state.phase === "finished"
          ? "A tale for the ages"
          : "The road through Middle-earth";
    $("room-badge").textContent = state.room;
    $("phase-label").textContent =
      state.phase === "lobby"
        ? "Waiting for companions"
        : state.phase === "finished"
          ? "Adventure complete"
          : state.players[state.turn].name + "'s turn";
    $("invite").value =
      location.origin +
      location.pathname +
      "?room=" +
      encodeURIComponent(state.room);
    renderPlayers();
    renderEncounter();
    renderHand();
    renderCharacter();
    $("log").innerHTML = [...state.log]
      .reverse()
      .map((line) => `<li>${esc(line)}</li>`)
      .join("");
  }
  renderLive();
}
async function act(action, card = -1, target = -1) {
  if (busy || !state) return;
  busy = true;
  clearTimeout(pollTimer);
  // Capture the revision before the request; server rejects stale double clicks.
  const revision = state.revision;
  render(true);
  try {
    state = await request("action/" + room, { action, card, target, revision });
  } catch (error) {
    notify(
      error.message +
        (!error.status ? " Check the table before trying again." : ""),
      true,
    );
    // Never retry a mutation automatically: it might already have succeeded.
    try {
      state = await request("state/" + room);
    } catch {
      /* Polling will resume and try reading again. */
    }
  } finally {
    busy = false;
    render(true);
    schedulePoll();
  }
}
function schedulePoll() {
  clearTimeout(pollTimer);
  if (state) pollTimer = setTimeout(poll, 1500);
}
async function poll() {
  if (!state || busy) return schedulePoll();
  const code = room; // Ignore a response if the user switched tables during the fetch.
  try {
    const data = await request("state/" + code);
    if (room === code && state && !busy && data.revision >= state.revision) {
      state = data;
      render();
    }
  } catch (error) {
    if (room !== code) return;
    if ([401, 404].includes(error.status)) {
      showEntry();
      notify(error.message, true);
      return;
    }
    $("connection").textContent = "Reconnecting… keep this tab open";
  } finally {
    schedulePoll();
  }
}

// SETUP: listeners are installed once. Dynamic buttons use event delegation.
$("character").addEventListener("change", preview);
$("join-form").addEventListener("submit", join);
$("room-code").addEventListener("keydown", (event) => {
  if (event.key === "Enter") {
    event.preventDefault();
    $("join-form").requestSubmit($("join-room"));
  }
});
$("table").addEventListener("click", (event) => {
  const control = event.target.closest("[data-action]");
  if (!control || control.disabled) return;
  if (
    control.dataset.action === "remove" &&
    !confirm("Remove this companion from the lobby?")
  )
    return;
  act(
    control.dataset.action,
    Number(control.dataset.card ?? -1),
    Number(control.dataset.target ?? -1),
  );
});
$("share").addEventListener("click", async () => {
  try {
    await navigator.clipboard.writeText($("invite").value);
    notify("Invite link copied. Send it to your companions!");
  } catch {
    $("invite").focus();
    $("invite").select();
    notify("Copy the selected invite link and send it to your friends.");
  }
});
$("switch").addEventListener("click", () => {
  if (busy) return;
  if (
    confirm(
      "Switch tables? Your existing seat stays reserved. Reopen its invite link in this tab to return.",
    )
  )
    showEntry();
});
$("rules-open").addEventListener("click", () => $("rules").showModal());
$("rules-close").addEventListener("click", () => $("rules").close());
$("retry").addEventListener("click", boot);
document.addEventListener("visibilitychange", () => {
  if (!document.hidden && state && !busy) {
    clearTimeout(pollTimer);
    poll();
  }
});
async function boot() {
  $("retry").hidden = true;
  try {
    catalog = await request("catalog", undefined, false);
    $("character").innerHTML = catalog.characters
      .map(
        (c, i) =>
          `<option value="${i}">${esc(c.name)} — ${esc(c.title)}</option>`,
      )
      .join("");
    preview();
    const code = (
      new URLSearchParams(location.search).get("room") || ""
    ).toUpperCase();
    $("room-code").value = /^[A-F0-9]{6}$/.test(code) ? code : "";
    token = savedSeat(code);
    if (token && /^[A-F0-9]{6}$/.test(code)) {
      room = code;
      try {
        setTable(await request("state/" + room));
        schedulePoll();
        return;
      } catch (error) {
        if (![401, 404].includes(error.status)) throw error;
        notify(error.message, true);
      }
    }
    $("loading").hidden = true;
    $("entry").hidden = false;
    state = null;
    room = "";
    token = "";
  } catch (error) {
    notify(error.message, true);
    $("retry").hidden = false;
  }
}
boot();
