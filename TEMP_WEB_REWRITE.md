# MTU web UI rewrite - working context (WIP handoff note)

Started 2026-11-16. This is a scratch document capturing the requirements, decisions and
current implementation state of the web UI rewrite. Delete or fold into AGENTS.md when done.

## 1. Original requirements

1. Drop the CSS framework (PicoCSS, `data/www/pico.min.css`, 82 KB) and write the CSS ourselves.
2. Make `/` work - currently it 404s (`serveStatic("/", SPIFFS, "/www/")` + no `index.html`).
3. Regroup the pages so they match the actual features:
   - a) state (current readings)
   - b) logs
   - c) settings (load/save JSON for now, real UI later)
   - d) debug (special hw calls, motor control)

Decisions taken during the work:

| Question | Decision |
|---|---|
| What is `/` | Server-rendered dashboard (status chips + sim data + link cards) |
| Theme | Dark, but not near black. "Instrument panel": slate background, amber accent |
| Motor control | Own top-level **Motors** tab, not buried in Debug |
| Logs | Stream live over the existing WebSocket instead of render-on-request only |

## 2. Page / route map

| Nav label | URL | Body source | JS |
|---|---|---|---|
| Home | `/` | generated in C++ (`buildDashboardBody()`) | `app.js` only |
| State | `/state` | `data/www/state.html` | `state.js` |
| Simulator | `/sim` | `data/www/sim.html` | `sim.js` |
| Logs | `/logs` | generated in C++ (`handleLogsRequest`) | `logs.js` |
| Settings | `/settings` | `data/www/settings.html` | `settings.js` |
| Motors | `/motors` | `data/www/motors.html` | `motors.js` |
| Debug | `/debug` | `data/www/debug.html` | `debug.js` |

Legacy URLs return 301 (defined in `legacyRedirects` in `src/web.cpp`):
`/log` -> `/logs`, `/sensors` -> `/state`, `/config` -> `/settings`, `/motor` -> `/motors`, `/about` -> `/debug`.

Endpoints:
- `/api/state` GET/POST - unchanged.
- `/api/motors` POST - unchanged (`stopAll`, `enable`, `disable`, `runAtSpeed`, `runSteps`, `moveToPosition`).
- `/api/i2c-scan?index=`, `/api/debug?index=` POST - unchanged, now used by the Debug page.
- `/heap` GET - kept, used by the Debug page.
- `/hw` GET - **removed** (it returned "Hello, world").
- `/ws` WebSocket - message format changed, see below.
- `server.serveStatic("/", SPIFFS, "/www/")` still serves `style.css`, `app.js` and the page
  `.html`/`.js` files; it is registered **after** the explicit routes so `/` is not swallowed
  by the static handler.

## 3. WebSocket protocol (changed)

Old: every 500 ms a bare transient-state JSON object was sent; logs were never streamed
(`notifyClients()` existed but was never called).

New envelope, one object per message:

```json
{"type":"state","data":{ ...transient state... }}     // every 500 ms to all clients
{"type":"logs","data":[ ... ]}                          // sent once on WS connect (snapshot)
{"type":"logDelta","data":[ ... ]}                      // entries since that client's cursor
```

Each client keeps a cursor in `logCursor` (map of client id -> logger sequence number), set on
`WS_EVT_CONNECT` and erased on disconnect, so no client receives duplicates and none loses
entries. `broadcastUpdates()` (renamed from `broadcastSensorData()`, declared in `include/web.h`,
called by the 500 ms task in `src/api.cpp`) sends state to everyone and then the per-client log
diff.

Supporting change in `Logger` (`include/Logger.h`, `src/Logger.cpp`):
- `m_total` monotonic counter incremented in `log()`
- `uint32_t total() const`
- `std::vector<std::string> getLogsSince(uint32_t sequence) const` - clamps to the 100-entry ring

Also added to transient state JSON (`src/state.cpp`, `TransientState::reportState()`):
`simData.ageMs` - milliseconds since the last simulator update (raw `lastUpdate` is millis since
boot, useless for display).

## 4. Files and what changed

**Removed**: `data/www/pico.min.css`, `data/www/scripts.js` (was empty), `about.html`,
`config.html`, `config.js`, `motor.html`, `motor.js`, `sensors.html`, `sensors.js`.
`data/www` is now 23.6 KB total (was ~107 KB with PicoCSS).

**New `data/www/style.css`** (~6.9 KB, no external requests):
- tokens: `--bg #141b24`, `--panel #212a36`, `--panel-raised`, `--line #303b49`,
  text `#e8eff7` / dim `#93a1b1` / faint `#8b96a4`, accent `#f2b54c` (amber), live `#43c9b0`
  (teal), ok `#6fbf73`, warn `#e8b45c`, danger `#e0555e`
- layout: sticky `.topbar` with brand + nav pills (`aria-current="page"` marks the active tab),
  `main` max-width 980 px, `.panel` cards, `.footbar` with a WS connection chip
- tables: uppercase micro-headers, zebra rows, right-aligned monospace numbers (`td.num`,
  `td.value` teal)
- controls: `button.primary` (amber), `button.danger` (red), `button.ghost`, dark inputs,
  `details` accordions, `.grid`, `.row`, `.chips`/`.chip` with `.dot on|warn|off`
- `.logs` monospace list with `.stamp` / `.msg`, `li.warn` highlighted
- `.cards` dashboard link tiles
- responsive to 420 px, `prefers-reduced-motion` respected
- measured contrast: panel text 12.5:1, nav 5.5:1, active tab 9:1, table headers 4.7:1

**New `data/www/app.js`** (script tags are emitted at the end of `<body>`, app.js first, then the page script): opens the WS, updates
the footer connection chip, parses the envelope, dispatches to handlers registered with
`MTU.on('state'|'logs'|'logDelta', fn)`, exposes `MTU.post(url, body)` and `MTU.state()`.
Reconnects after 2 s.

**`src/web.cpp`** (rewritten, 312 lines): single `pageTemplate` raw string instead of five
stitched fragments (`html1`..`html5`); `Page` struct with the new page list; helpers
`escapeHtml`, `formatUptime`, `chip`, `logsJson`, `logListItem`; `buildDashboardBody()`;
`handleLogsRequest()`; WS event handler sends snapshot + state on connect; `broadcastUpdates()`.
Dashboard shows uptime, heap, min heap, CPU, IP, HID, 12V, sim data table, per-motor
enabled/disabled chips and five link cards.

**`src/state.cpp`**: `ageMs` added to the sim data JSON.
**`include/Logger.h` / `src/Logger.cpp`**: `total()`, `getLogsSince()`.
**`include/web.h`**: `broadcastSensorData()` renamed to `broadcastUpdates()`.
**`src/api.cpp`**: task calls `broadcastUpdates()`.

## 5. State right now (evening of 2026-11-16, nothing pending)

Both uploads are done - device and working tree match. No further upload is needed before
continuing.

Commands used (for reference):
- firmware: `platformio run -e esp32-s3-ota -t upload` (Flash 38.6 %, RAM 21.6 %)
- web files: `platformio run --target uploadfsota --environment esp32-s3-ota`
  (VS Code menu "Upload filesystem Image OTA")

Verified live on `http://mtu.local`:

| Check | Result |
|---|---|
| `/` dashboard | 200 - status chips, sim table, motor chips, 5 link cards |
| `/state` | live values over WS (raw 266, calibrated 70, button bit 8, footer "connected") |
| `/logs` | 200, server-rendered ring of ~75 entries |
| `/settings` | loads the persisted JSON, "configuration loaded" |
| `/motors` | radio selection + "Run" -> feedback `OK` |
| `/debug` | heap check returns 197400 bytes |
| `/sensors` `/config` `/motor` `/log` `/about` | 302 to the new pages |
| `/hw` | 404 (removed), `/heap` still works |
| WebSocket | snapshot `{"type":"logs","data":[75]}`, `state` every 500 ms, real `logDelta` seen |
| PicoCSS | not referenced anywhere, `style.css` 7.5 KB served |

Not yet checked (do these first tomorrow):
1. `/logs` rendering - the snapshot now uses key `data`, which `app.js` unwraps, so entries
   should render and then append. Before the key fix the list was cleared and stayed empty;
   that is the only known regression and it is already fixed in the flashed firmware, but the
   browser rendering was not re-run.
2. Motors enable/disable/stop-all and steps/position commands (not exercised - they move real
   levers). Only `runAtSpeed` with speed 0 was sent.
3. Settings save + factory reset (not exercised to avoid touching the stored config).
4. Debug i2c scan and debug call (not exercised).
5. Responsive look on a phone.

## 6. Gotchas hit so far

- `sed -i` on Windows through this shell failed and **deleted** `src/web.cpp`; it was restored
  with `git checkout -- src/web.cpp` and rewritten with `write`/`edit`. Prefer the edit tool
  over sed here.
- Arduino `String` does not convert to `std::string`; mixing `"<li" + "..."` (const char* + const char*)
  is invalid - wrap with `std::string(...)` or use `std::to_string(...)`.
- `server.on("/")` must be registered **before** `serveStatic` or the static handler wins.
- Page scripts placed in `<head>` run before the DOM exists, so `motors.js`, `logs.js` and
  `settings.js` silently failed to attach their handlers (the old code hid this with a
  `DOMContentLoaded` wrapper). Scripts are now emitted after the footer.
- PlatformIO does not produce a `spiffs.bin` for this project, so the `data/www` files have to be
  pushed to SPIFFS separately. That upload is done by the owner (two commands: one for the
  firmware, one for SPIFFS) - the rewrite does not move any static HTML/JS into C++ string
  constants to work around it.

## 7. Where to continue tomorrow

1. Open `http://mtu.local/logs` in a browser and confirm entries render and new ones appear
   within a few seconds (the axis-sampling line logs every 10 s).
2. Decide on the Settings UI: keep the raw JSON textarea or build real per-axis / per-motor
   forms (axis min/max/reversed, motor current/microsteps/speed multiplier, HID toggle,
   trim wheel and speed brake switches).
3. Optional cleanups: log level filter, `/heap` moved under `/api`, block direct
   `*.html` access from `serveStatic`, live-updating dashboard instead of a load-time snapshot.
4. Nothing is committed yet - the rewrite is uncommitted in the working tree
   (`git status` shows the deleted old pages and the new files).

## 8. Reboot investigation ("ESP restarts every 2-3 minutes")

Measured with read-only polling of `/` (uptime + heap) before any code change:

| scenario | result |
|---|---|
| HTTP every 6 s, no websocket | reboot at uptime ~2:30 |
| HTTP every 30 s, no websocket | reboot twice in 6 min, ~170 s apart |
| no requests at all for 6 min | reboot, uptime read 1:57 at the end |
| websocket kept open | connection died (code 1006, no close frame) at ~104 s |

Free heap stayed at ~195-215 KB in every run, so this is **not** a memory leak - it is a
crash/panic. The interval is tied to *uptime*, not to request rate.

Found bug (in the new code, now fixed): `Logger::getLogsSince()` computed the ring base index
as `m_total - m_logs.size()` instead of `... + 1`. Once more than 100 entries had ever been
logged (about a minute after boot, because the MCP23017 poll and axis sampler log constantly),
the loop read `m_logs[size]` - one element past the end of a `std::deque`. Out-of-bounds read
on every 500 ms broadcast to every websocket client.

Also hardened: ESPAsyncWebServer caps the per-client websocket send queue at
`WS_MAX_QUEUED_MESSAGES` (32). `broadcastUpdates()` now checks `availableForWriteAll()` /
`client.queueIsFull()` instead of pushing into a full queue.

Diagnostic added so the cause of any future silent reboot is visible in `/logs` and on the
dashboard: `resetReason()` (`esp_reset_reason()`) is logged as the first line of every boot and
shown as a "last reset" chip on the Home page - panic, task watchdog, interrupt watchdog,
brownout, etc.

After flashing that build the device ran 12+ minutes with no reboot.

**Regression found afterwards:** the queue guard `ws.availableForWriteAll()` was wrong. This
fork of ESPAsyncWebServer never calls `cleanupClients()` internally, so clients whose socket
died stay in `_clients` with status `WS_DISCONNECTED`, and `queueIsFull()` returns true for
them. `availableForWriteAll()` then permanently returned false and the `state` broadcast was
skipped - the State page stopped updating live values. Fixed by dropping the global gate,
calling `ws.cleanupClients()` at the top of each tick and sending `state` + `logDelta`
per connected client. Verified: 60 `state` messages in 30 s (2/s, as intended).

Why the out-of-bounds read mattered: `std::deque` allocates in 512-element blocks, so
`m_logs[100]` on a 100-element deque does not throw - it returns an unset slot, which is
copied into a `std::string` with a garbage pointer and freed on destruction. That is a panic,
which matches a silent reboot every ~2.5 minutes.

## 9. Round 2 design changes (2026-11-17, after the crash fix)

1. **Logs newest first** - `handleLogsRequest` renders the ring in reverse order and the lede
   says so; `logs.js` prepends streamed entries and trims from the bottom (`MAX_LINES` 200).
2. **Simulator data split out** - the sim table was removed from both the Home dashboard and the
   State page. New nav entry **Simulator** (`/sim`, `data/www/sim.html` + `sim.js`) shows only
   what the unit is receiving: throttle 1/2 (raw + percent), speed brake (raw + percent), trim,
   parking brake, auto throttle and "last packet N ms ago". The page carries a note that a mock
   data form (POST to an API so the motors can be exercised without the simulator) is still to
   be built.
3. **Buttons are no longer checkboxes** - `<span class="switch">` boxes with a thin border that
   fill with the teal `--live` colour when the bit is set; `state.js` toggles the `on` class from
   `buttonsRawValue`.

## 10. Round 3 (2026-11-17)

1. **Page titles** - dropped the `MTU - ` prefix from every page title (`pages` in `src/web.cpp`,
   plus the two hardcoded ones in `handleLogsRequest` and the `/` handler). `generateNav()` reuses
   the same strings, so the nav tabs now read Home / State / Simulator / Logs / Settings / Motors /
   Debug; the brand in the top bar still carries "MTU".
2. **State page button labels** - the labels inherited from the old `sensors.html` were the README
   list in bit order and are wrong. Verified by the owner against the physical quadrant:

| bit | switch | note |
|---|---|---|
| 1 | TOGA 1 | |
| 2 | AT disconnect 1 | |
| 3 | fuel cutoff 1 | part of `reversedButtonsMask` (3, 11) |
| 4 | parking brake | also drives the parking LED in `src/pins.cpp` |
| 5 | trim indicator end stop 2 | firmware calls this one "end stop 1" in `TrimDriver` |
| 6 | horn cutoff | |
| 8 | stab trim AP | the reverse-value gate in `src/joy.cpp` |
| 9 | TOGA 2 | |
| 10 | AT disconnect 2 | |
| 11 | fuel cutoff 2 | part of `reversedButtonsMask` |
| 12 | stab trim main | the "12V present" label here was wrong; 12V is GPIO12 |
| 0, 7 | not identified | polled, no switch matched |
| 13-15 | nothing | rotary encoder is GPIO16/17/18, not the MCP inputs |

Ordering on the page follows the HID button the switch becomes (`joy.cpp` sends bit `i` to joystick
button `i + 1`); the end stops are listed last. Idle `buttonsRawValue` is 6424 = bits 3, 4, 8, 11,
12 - fuel cutoff 1, parking brake, stab trim AP, fuel cutoff 2, stab trim main, which lines up with
the table. Firmware untouched; only the HTML changed.
