# TV Transmission

A terminal UI (and CLI) client for Transmission (`transmission-daemon`),
built on [Turbo Vision (magiblot/tvision)](https://github.com/magiblot/tvision),
written in C++17.

It talks to `transmission-daemon` over its JSON RPC (HTTP, port 9091 by
default), so no native Transmission library is needed — just libcurl for
HTTP and nlohmann/json for parsing.

## Features

**Torrent list (main window)**
- Always maximized and locked: can't be moved, resized, zoomed or
  closed (it's the app's main view, kept as a raw pointer internally —
  see "Fixed bugs" below for why closing it used to crash the app)
- Columns: name, % done, size, download rate, upload rate, date added,
  status
- Click a column header to sort by it; click again to reverse the
  direction (a `^`/`v` indicator shows the active column and direction);
  the chosen column and direction are saved and restored on the next launch
- Fixed colors (white text on blue background; the current row is black
  on white) regardless of the terminal's overall color scheme
- Double-click a row to open a details window for that torrent — window
  title includes the start of the torrent's name, so several open ones
  are distinguishable at a glance; name, size, %, rates, date added,
  status, error if any, plus a speed-limit override — see below; these
  are ordinary, non-modal windows using tvision's default palette (same
  look as the Settings dialog), so you can have several open at once,
  and double-clicking a torrent that already has one open brings it to
  the front instead of opening a duplicate

**Per-torrent speed limit override**
- In a torrent's details window, three independent checkboxes:
  - "Limit download" / "Limit upload", each with a KB/s field — caps
    just this torrent's speed in that direction
  - "Honor global speed limits" — whether this torrent follows the
    session's global limit (see below) at all
  - These are genuinely independent in Transmission: a torrent can have
    no speed limit of its own and still ignore the global limit if it
    doesn't "honor" it, so unchecking the first two doesn't by itself
    mean "use the global limit" — the third checkbox is what controls that
- "Apply" sends the change immediately (`torrent-set` RPC); "Close"
  closes the window without changing anything

**Tracker details**
- A "Trackers..." button in the torrent details window opens a
  separate, non-modal table listing every tracker for that torrent:
  host, tier, seeders, leechers, downloaded count, and a short status
  (OK/Error). Turbo Vision has no tab control, so this is a dedicated
  window rather than a second tab on the details dialog (see
  `TrackerListWindow`)
- This data (`trackerStats`, part of `torrent-get`) isn't fetched as
  part of the regular list refresh; it's requested only when this
  window is opened, and again only when you press its own "Refresh"
  button (no auto-refresh)
- Double-click a tracker row for a small window with that tracker's
  full status: last/next announce time and the complete error or
  success message, which don't fit in a table row

**Managing torrents**
- Add a torrent from a magnet link, `.torrent` URL, or local path (F2)
- Start / stop the selected torrent (F5 / F6)
- Remove the selected torrent (F8) — keeps its files on disk
- Delete the selected torrent **and its files on disk** — a separate,
  clearly distinct action from Remove, reachable from the Torrent menu
  and the right-click context menu (no default keyboard shortcut, given
  how destructive it is)
- Both Remove and Delete-with-files ask for confirmation first
  (`messageBox`, showing the torrent's name) before doing anything —
  Delete's confirmation spells out that the operation can't be undone
- Right-click a row for a context menu: Start, Start Now, Stop, Verify,
  Reannounce, Remove, Delete (with files), Details — right-clicking
  also selects that row first, even if it wasn't already focused
- Start Now (bypasses the download queue), Verify (rechecks local data
  against piece hashes), and Reannounce (asks trackers for more peers
  right away) — also reachable from the Torrent menu, not just the
  context menu
- Every one of these actions is enabled or disabled based on the
  selected torrent's current state (e.g. Start is disabled while
  already running, Stop is disabled while stopped, Reannounce only
  makes sense while active) — and this is a single shared state, so
  disabling an action grays it out everywhere it appears at once (the
  Torrent menu, the status bar, and the context menu) rather than each
  needing to be kept in sync separately
- A status bar shows the combined download/upload rate across all
  torrents, refreshed on every UI tick (no extra RPC calls)

**Settings (F9)**
- Refresh interval (seconds), host, port, RPC username/password,
  interface language — applied immediately and saved to disk (see
  "Configuration file" below for where and how)
- Global (session-wide) download/upload speed limits — read from and
  written straight to the Transmission daemon itself (`session-get` /
  `session-set`), not stored in this app's own settings file; these are
  the defaults any torrent without its own override (above) follows

**Input validation**
- Every numeric field (refresh interval, RPC port, global and
  per-torrent KB/s limits) uses tvision's `TRangeValidator`: non-digit
  keystrokes are rejected as you type, and confirming with an
  out-of-range value shows an error instead of silently accepting it.
  Refresh interval: 1-86400 seconds; port: 1-65535; speed limits:
  0-1,000,000 KB/s

**Window management**
- Standard menu: Zoom, Next, Close, Tile, Cascade, and a "Window list"
  dialog (Alt+0) listing every open window, letting you jump to one

**Internationalization**
- English (default) and Italian, selectable from the Settings dialog
- Windows and dialogs that get rebuilt each time they're shown (Add
  torrent, Settings, Torrent details, the main list's title) update
  immediately; the menu bar and status bar are only built once at
  startup and relabel on the next restart (see "Known limitations")

**Command-line interface**

Any argument on the command line switches to a non-interactive CLI
instead of launching the TUI:

```
tv-transmission list
tv-transmission add 'magnet:?xt=urn:btih:...'
tv-transmission start 12
tv-transmission stop 12
tv-transmission remove 12 --delete-data
```

By default host/port/user/password come from the same saved settings
file the TUI uses, so day-to-day commands don't need any connection
flags. Override any of them per-invocation:

```
tv-transmission --host seedbox.example.org --port 9091 \
                 --user vanni --password '...' list
```

Run `tv-transmission --help` (or `-h`, or with no arguments at all for
the interactive TUI) for the full command reference.

## How the connection works

There's no persistent connection: every action (refreshing the list,
adding/starting/stopping/removing a torrent) opens a fresh HTTP request
to `host:port` with the current credentials. Changing settings from the
TUI applies them immediately and triggers a refresh right away, so a
mistake shows up at once (an empty list); the CLI's `list` command
distinguishes a genuinely empty torrent list from a failed connection
via the RPC client's last-error state, and exits non-zero on failure.

## Configuration file

Settings (host, port, user, password, refresh interval, language, and
the torrent list's last sort column/direction) are stored in:

```
$XDG_CONFIG_HOME/tv-transmission/settings.json
```

or `~/.config/tv-transmission/settings.json` if `XDG_CONFIG_HOME` isn't
set. The file is read at startup and rewritten every time you confirm
the Settings dialog, or click a column header to sort by it (or, from
the CLI, whenever you'd change them from the TUI — the CLI itself is
read-only with respect to this file).

Global and per-torrent speed limits are **not** in this file — they
live on the Transmission daemon itself and are read/written through the
RPC (`session-get`/`session-set`, `torrent-set`), so they're shared with
any other client talking to the same daemon.

**About the password:** the file is written with `0600` permissions
(only the current user can read it) — that's the actual protection.
On top of that, the RPC password is obfuscated (XORed with a key
derived from `/etc/machine-id` + `$HOME`, then base64-encoded) rather
than stored in plain text, so it doesn't show up in clear if you `cat`
the file, paste it into a support ticket, or someone glances at your
screen. **This is not real encryption**: the key is derived entirely
from data anyone with the same local access already has, so it's
reversible by design, not resistant to a determined local attacker —
real protection would mean an OS-level secret store (e.g. libsecret),
not implemented here because it needs a keyring daemon that typically
isn't available on the headless/SSH-managed servers this app is often
used from.

## Building

1. Add tvision as a submodule:
   ```
   git submodule add https://github.com/magiblot/tvision external/tvision
   git submodule update --init --recursive
   ```
2. Install dependencies (Debian/Ubuntu):
   ```
   sudo apt install cmake libcurl4-openssl-dev libncursesw5-dev libgpm-dev
   ```
   (`nlohmann-json3-dev` is optional — if missing, it's downloaded from
   GitHub automatically via CMake's FetchContent.)
3. Build:
   ```
   mkdir build && cd build
   cmake ..
   cmake --build . -j
   ./src/tv-transmission
   ```

Running `cmake .` directly in the project root (instead of a `build/`
subdirectory) without `external/tvision` present yet stops with an
explicit error pointing at step 1 above — that's the intended check,
not a bug.

## Packaging as an AppImage

A self-contained, portable Linux binary — download it, `chmod +x`, run,
no installation or matching system libraries required (chosen over
Flatpak: this is a terminal tool, and Flatpak's sandboxing model adds
friction — explicit filesystem permissions, launching via `flatpak run`
— that doesn't fit a program meant to be invoked directly from a shell).

```
./packaging/appimage/build-appimage.sh
```

On first run this downloads `linuxdeploy` and `appimagetool` (cached
under `packaging/appimage/tools/`, gitignored — not something to commit
to the repo), builds the project in Release mode, bundles the binary
together with its shared library dependencies (libcurl, libncursesw,
libgpm, and libcurl's own dependency tree — `libc`, `libstdc++`,
`libgcc_s`, `libm` and a few other core system libraries are assumed
present on any target and deliberately left out), and produces:

```
build/TvTransmission-x86_64.AppImage
```

which runs the TUI with no arguments, or the CLI with any (see
"Command-line interface" above) — same as the plain binary. The
`.desktop` file bundled inside sets `Terminal=true`, so double-clicking
the AppImage from a file manager opens a terminal rather than doing
nothing (this is a terminal app, not a windowed one).

`packaging/appimage/tv-transmission.png` is a placeholder icon —
replace it with a real one if you want (any size works; `linuxdeploy`
handles the icon theme directories).

## Project layout

```
CMakeLists.txt              Top-level build (adds tvision + dependencies)
src/
  main.cpp                  Entry point: loads settings, dispatches to CLI or TUI
  AppSettings.h              Settings struct + Language enum
  Config.h/.cpp              Load/save settings.json
  Obfuscation.h/.cpp          Password obfuscation for settings.json (not real encryption)
  TextUtil.h/.cpp             Shared UTF-8-safe pad/truncate helper
  cli/
    Cli.h/.cpp                Command-line interface
  rpc/
    Torrent.h                 Torrent data struct
    Tracker.h                 Per-tracker stats struct (trackerStats)
    TransmissionClient.h/.cpp  Minimal Transmission RPC client (libcurl + nlohmann/json)
  ui/
    App.h/.cpp                 TApplication subclass: menu bar, status bar, event dispatch
    Strings.h/.cpp              Translation strings (English/Italian)
    TorrentListWindow.h/.cpp    Main window: list, header, sorting, colors, context menu
    TorrentDetailsWindow.h/.cpp Per-torrent details window
    TrackerListWindow.h/.cpp    Per-torrent tracker table (opened from the details window)
    TrackerDetailWindow.h/.cpp  Full status for a single tracker (double-click a row)
    AddTorrentDialog.h/.cpp     "Add torrent" dialog
    SettingsDialog.h/.cpp       "Settings" dialog
    WindowListDialog.h/.cpp     "Window list" dialog
    BandwidthStatusLine.h/.cpp  Status bar with a runtime-updatable item
packaging/
  appimage/
    build-appimage.sh          Builds build/TvTransmission-x86_64.AppImage
    tv-transmission.desktop     Desktop entry (Terminal=true)
    tv-transmission.png         Placeholder icon
```

## Known limitations

- RPC password on disk is obfuscated, not really encrypted — see
  "Configuration file" above for exactly what that does and doesn't
  protect against.
- No network-error UI: `TransmissionClient::lastError()` exists but
  isn't yet surfaced anywhere in the TUI (a `messageBox` or a status
  area would be the natural place).
- Changing the language relabels every window/dialog immediately
  *except* the menu bar and status bar, which only pick up the new
  language on the next app restart (see "Internationalization" above).
- Speed limits (global and per-torrent) are only exposed in the TUI so
  far — the CLI has no equivalent of the Settings dialog's or details
  window's speed-limit controls.
- The CLI's `remove --delete-data` does not ask for confirmation (unlike
  the TUI's Remove/Delete actions) — intentional, so it stays usable in
  scripts, but worth keeping in mind since it's the one place this app
  deletes files without a prompt.

## Possible future additions

Transmission's RPC exposes more than this client currently uses. Not
implemented yet, but straightforward to add along the same lines as the
actions above:

- **Queue reordering** — move a torrent to the top/bottom of the
  download queue, or up/down one position (`queue-move-top`/`-up`/
  `-down`/`-bottom`)
- **Change download location** for an already-added torrent
  (`torrent-set-location`)
- **Rename a file or folder** inside a torrent (`torrent-rename-path`)
- **Per-torrent seed ratio limit**, distinct from a speed limit
  (`seedRatioLimit`/`seedRatioMode` in `torrent-set`)
- **Per-torrent bandwidth priority** (high/normal/low), distinct from
  the absolute KB/s limit already implemented (`bandwidthPriority`)
- **Incoming port test** (`port-test`) and **blocklist update**
  (`blocklist-update`) — session-level, would fit in the Settings dialog
- **Free disk space** for a given path (`free-space`) — useful before
  adding a large torrent

## Fixed bugs

Kept here for context, in case similar patterns come up again.

**Password stored in plain text.** Fixed by obfuscating it (XOR with a
machine+user-derived key, base64-encoded) before writing settings.json
— see "Configuration file" above for exactly what this does and
doesn't protect against (short version: not real encryption, but the
password no longer shows up in clear in the file). No migration from
older config files with a plain-text password: the field is now always
assumed to be in the obfuscated format, so a pre-existing plain-text
password won't be read back correctly — delete `settings.json` (or
just re-enter the password in the Settings dialog once) after updating.

**Password field had no masking.** tvision's `TInputLine` has no
built-in flag for this (checked: nothing in `dialogs.h`/`tinputli.cpp`
resembles one). Fixed with `PasswordInputLine`, a subclass that
temporarily swaps the (public) `data` member for a same-length string
of asterisks only for the duration of the base class's own `draw()`
call, then restores the real value immediately after — every other
method (`getData`/`setData`/the validator/`handleEvent`) keeps
operating on the real, unmasked value as normal. Reuses `TInputLine::
draw()`'s own scrolling/selection-highlight logic entirely instead of
reimplementing it.

**Details window and Settings dialog colors, reverted — then actually
matched.** These went through a few iterations: the details window got
fixed yellow-on-black colors, then yellow-on-blue; the Settings dialog
got the same yellow-on-blue applied to match it. All-yellow-on-blue for
every element (including buttons and checkboxes, which lose their
distinct look when every color index resolves to the same fixed value)
turned out flatter and less readable than tvision's own default
palette — the one the Settings dialog had from the start, which the
user liked — so both color overrides were removed. That still wasn't
enough, though: with no override, the details window (a `TWindow`) and
the Settings dialog (a `TDialog`) turned out visibly different anyway
(e.g. red buttons vs. green) — verified directly, comparing
`mapColor()` output for the same 20 color indices on a plain `TWindow`
and a plain `TDialog`: all 20 diverged. `TWindow` and `TDialog` each
ship their own default `getPalette()` (`cpBlueWindow`/`cpCyanWindow`/
`cpGrayWindow` vs. `cpGrayDialog`/`cpBlueDialog`/`cpCyanDialog`, see
tvision's `twindow.cpp`/`tdialog.cpp`), assigning different final
colors to the same slots — "use the default palette" isn't one thing
in tvision, it depends on which of these two base classes a window
derives from. Fixed by making `TorrentDetailsWindow` inherit from
`TDialog` instead of `TWindow`: same class as the Settings dialog, same
default palette, verified to produce identical colors for all 20
indices tested (it runs non-modally, inserted into the desktop like any
other window rather than via `execView()`, so `TDialog`'s Esc/Enter
shortcuts — which only act while modal — are harmless here).

**"Honor global speed limits" label cut off in the details window.**
`TCluster` (the base of `TCheckBoxes`) draws each item as a 5-column
"[ ] " prefix followed by the label, so the rect needs to be at least 5
plus the longest label's length — the checkbox rect was sized for the
two short labels ("Limit download"/"Limit upload") from before this
third, longer checkbox existed, and was never widened when it was
added, so "Honor global speed limits" got cut to "Honor global speed
limi". Fixed by widening the rect (with room to spare) and moving the
KB/s input fields further right to match.

**"Honor global speed limits" wasn't user-controllable.** The first fix
for the per-torrent override always sent Transmission's
`honorsSessionLimits` flag as `true` whenever speed limits were
applied, reasoning that the UI had no control for it and this was the
closer-to-correct default. That removed a real, useful choice:
`honorsSessionLimits` — distinct from `downloadLimited`/`uploadLimited`
— decides whether a torrent respects the session's global limit *at
all*, independently of whether it has its own per-torrent limit; a
torrent can have no override of its own and still ignore the global
limit entirely if this is false. Simply clearing the per-torrent
override (setting `downloadLimited`/`uploadLimited` to false) does not
by itself mean "use the global limit" — that's what this flag is for.
Fixed by exposing it as its own checkbox ("Honor global speed limits")
in the details window, alongside the two limit overrides, instead of
hardcoding it to `true`.

**The "Close" button in the torrent details window did nothing.**
`TButton::press()` sends its command with `event.message.infoPtr` set
to the button itself (see tvision's `tbutton.cpp`), but
`TWindow::handleEvent`'s own `cmClose` handling only reacts when
`infoPtr` is `0` or the window itself — a `cmClose` sent by a button
inside the window doesn't match either, so it was silently dropped.
Fixed by using a dedicated command for the button and calling
`close()` directly in this window's own `handleEvent()`, sidestepping
that check (this is the same `close()` `TWindow::handleEvent` would
have called anyway, had the check passed).

**Double-clicking a torrent that already had an open details window
opened a second one.** Fixed by searching the desktop's open windows
for an existing `TorrentDetailsWindow` with the same torrent id before
creating a new one (same `deskTop->last`/`next` traversal already used
for the "Window list" dialog); if found, it's brought to the front
(`select()`) instead. Clicking "Trackers..." for a torrent that already
had its tracker list open had the same problem, fixed the same way
(`TrackerListWindow::torrentId()`, checked in
`TorrentDetailsWindow::showTrackers()`).

**Column misalignment for very large torrents.** The size column always
showed the value in MB with a fixed 8-character field (`%8.1fMB`). That
field width silently assumed the number would never need more than 8
digits — which breaks for a large enough torrent: one around 7 TB shown
in MB needs 9 digits, one more than the field allowed, shifting every
column after it (download/upload rate) by the overflow amount. Same
root cause as the accented-name bug below: a fixed-width assumption that
doesn't hold for unbounded input. Fixed with an adaptive-unit formatter
(`formatSize()` in `TextUtil.h/.cpp`, shared with the CLI) that picks
KB/MB/GB/TB/PB so the number itself stays short (one decimal, generally
1-4 digits) regardless of how large the torrent is.

**Column misalignment for names with accented letters.** Each row used
to be built with printf's `%-30.30s`, which pads/truncates by *byte*
count. A name with accented letters (é, à, ò, ...) or other multi-byte
UTF-8 characters takes more bytes than displayed characters, so two
names of the same visual length could consume a different number of
bytes in that field — shifting every column after it (%, size, D:/U:)
by an amount that depended on the name. Fixed with a helper
(`padOrTruncateUtf8`, in `TextUtil.h/.cpp`) that pads/truncates by
Unicode codepoint instead of by byte.

**Hidden scrollbar.** The list view was created as wide as the entire
available rectangle, *including* the column reserved for the vertical
scrollbar; being drawn on top of it (inserted later, hence higher in
z-order), it covered the scrollbar completely. The scrollbar existed
and worked, it just wasn't visible. Fixed by narrowing the list's
rectangle by one column.

**Segfault on closing the main window.** `App::listWindow_` is a raw
pointer to the torrent list window; tvision's `TWindow::close()`
destroys the object (`destroy(this)`) when it receives `cmClose` and
has the `wfClose` flag set. Closing it left `App::listWindow_` dangling,
used on the very next idle tick (`refresh()`, `totalDownloadRate()`,
...) — a use-after-free. Fixed by clearing every window flag on the
main window's constructor (`flags = 0`): being the one window the app
keeps a direct pointer to, and meant to always stay maximized, it
shouldn't be closable, movable, resizable or zoomable in the first
place.

**Settings dialog fields silently swapped.** The dialog used to
"recover" its input fields after being closed by scanning the TGroup's
child list in the order encountered starting from `last`. That order
does not match insertion order: tvision's `TGroup::insert()` inserts
each new view at the head of a circular list, so scanning it gives the
*reverse* of insertion order. With 5 fields, host/user and
refresh-interval/password ended up swapped with each other (the port,
being the middle field, happened to look correct by pure coincidence).
Fixed by having the dialog return direct pointers to each field at
creation time (see `SettingsDialogFields` in `SettingsDialog.h`)
instead of reconstructing them afterwards — the same pattern is now
used for every other dialog with more than one input field.

**Window list dialog title showing literal tildes.** Menu and status
bar labels use `~x~` markup to underline a hotkey letter, interpreted
only by `TMenuItem`/`TStatusItem`/`TButton`. A plain `TDialog` title
does not strip that markup, so reusing a menu label as a dialog title
would have shown literal `~` characters in the title bar. Fixed with a
separate, markup-free string for the dialog title.

**CLI `list` silently succeeding against an unreachable daemon.**
`TransmissionClient::listTorrents()` returns an empty vector both when
the daemon genuinely has no torrents and when the request itself failed
(wrong host/port, daemon down, authentication failure, ...). The CLI
originally printed "No torrents." with exit code 0 in both cases,
making failures undetectable in scripts. Fixed by checking
`lastError()` when the result is empty, printing it to stderr and
exiting non-zero only when an actual error occurred.
