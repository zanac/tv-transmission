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

**Managing torrents**
- Add a torrent from a magnet link, `.torrent` URL, or local path (F2)
- Start / stop the selected torrent (F5 / F6)
- Remove the selected torrent (F8)
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
(only the current user can read it), but the RPC password is stored in
**plain text** — there's no encryption. Keep that in mind if you reuse
that password elsewhere, and be aware that any CLI command run under
your user account can read it just as the TUI does.

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

## Project layout

```
CMakeLists.txt              Top-level build (adds tvision + dependencies)
src/
  main.cpp                  Entry point: loads settings, dispatches to CLI or TUI
  AppSettings.h              Settings struct + Language enum
  Config.h/.cpp              Load/save settings.json
  TextUtil.h/.cpp             Shared UTF-8-safe pad/truncate helper
  cli/
    Cli.h/.cpp                Command-line interface
  rpc/
    Torrent.h                 Torrent data struct
    TransmissionClient.h/.cpp  Minimal Transmission RPC client (libcurl + nlohmann/json)
  ui/
    App.h/.cpp                 TApplication subclass: menu bar, status bar, event dispatch
    Strings.h/.cpp              Translation strings (English/Italian)
    TorrentListWindow.h/.cpp    Main window: list, header, sorting, colors
    TorrentDetailsWindow.h/.cpp Per-torrent details window
    AddTorrentDialog.h/.cpp     "Add torrent" dialog
    SettingsDialog.h/.cpp       "Settings" dialog
    WindowListDialog.h/.cpp     "Window list" dialog
    BandwidthStatusLine.h/.cpp  Status bar with a runtime-updatable item
```

## Known limitations

- The password field in the Settings dialog isn't masked (tvision has
  no built-in flag for `TInputLine`; it would need a dedicated
  subclass that substitutes typed characters with `*`).
- RPC password saved in plain text on disk (0600 permissions, no
  encryption) — see "Configuration file" above.
- No network-error UI: `TransmissionClient::lastError()` exists but
  isn't yet surfaced anywhere in the TUI (a `messageBox` or a status
  area would be the natural place).
- Changing the language relabels every window/dialog immediately
  *except* the menu bar and status bar, which only pick up the new
  language on the next app restart (see "Internationalization" above).
- Speed limits (global and per-torrent) are only exposed in the TUI so
  far — the CLI has no equivalent of the Settings dialog's or details
  window's speed-limit controls.

## Fixed bugs

Kept here for context, in case similar patterns come up again.

**Details window and Settings dialog colors, reverted.** These went
through a few iterations: the details window got fixed yellow-on-black
colors, then yellow-on-blue; the Settings dialog got the same
yellow-on-blue applied to match it. In the end, all-yellow-on-blue for
every element (including buttons and checkboxes, which lose their
distinct look when every color index resolves to the same fixed value)
turned out flatter and less readable than tvision's own default
palette — the one the Settings dialog had from the start, which the
user liked. Both windows are now back to that default; there's no
color override left in either.

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
(`select()`) instead.

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
