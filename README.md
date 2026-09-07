# TV Transmission

**Version 1.1** — stable release.

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
- Columns: name (bold), progress bar, size, download rate, upload
  rate, date added, status — shown by default; nine more are available
  but hidden by default (see "Manage columns..." below): ratio,
  all-time uploaded/downloaded totals, location, ETA, peers connected,
  queue position, bandwidth priority, and completion date
- Every column except the progress bar can be resized (drag the
  separator between two headers with the mouse) or reordered
  (double-click a column's name — "<"/">" markers appear where a move
  is possible); **Columns → Manage columns...** does the same, plus
  showing/hiding columns, all from one place — see its own entry below.
  Width, order, and visibility are all saved and restored across
  launches
- Click a column header to sort by it; click again to reverse the
  direction (a `^`/`v` indicator shows the active column and direction);
  the chosen column and direction are saved and restored on the next launch
- Progress bar fills up with block characters (`[████░░░░] 42%`) as the
  torrent approaches 100%, instead of a plain percentage number
- Text color reflects the torrent's status at a glance: cyan while
  downloading, green while seeding, gray while stopped, yellow while
  checking/queued, red if it has an error — all on the same blue
  background as before; the currently selected row stays black-on-white
  regardless of status, so it's always clear what's selected
- Double-click a row to open a details window for that torrent — window
  title includes the start of the torrent's name, so several open ones
  are distinguishable at a glance; these are ordinary, non-modal windows
  using tvision's default palette (same look as the Settings dialog), so
  you can have several open at once, and double-clicking a torrent that
  already has one open brings it to the front instead of opening a
  duplicate. Shows:
  - Name, size (with piece count/size), download location, public/
    private, magnet link (truncated — see "Known limitations" for why)
  - Completion %, availability % (see "Fixed bugs" below for the
    formula), all-time downloaded/uploaded totals and ratio, current
    download/upload rate, average speed since first started
  - Added date, last activity date
  - Time spent downloading / time spent seeding
  - Status, error if any, torrent ID
  - A speed-limit override — see below
  - Two short fields share each row wherever they naturally pair up
    (e.g. size + piece info, completion % + availability %, added date +
    last activity) instead of one field per line — this is what actually
    keeps the window a manageable height with this much information in
    it, rather than a single long column that overflows most terminals
  - This extra detail (everything past size/rates/status) is fetched
    with a separate RPC call made only when the window is opened, not
    as part of the main list's periodic refresh — see "How the
    connection works" below

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
  — typed directly, or via "Browse...", which opens tvision's own file
  dialog (filtered to `*.torrent`) and fills the field in with whatever
  gets picked; see "Fixed bugs" below for why Browse closes this dialog
  first and reopens it afterwards instead of opening the file dialog
  directly from a button inside it
- Adding a torrent that's already in the list shows a small "already
  present" popup instead of silently doing nothing — Transmission's own
  RPC distinguishes a genuinely new torrent from a duplicate in its
  response, so this doesn't need a manual comparison against the
  existing list, and it doesn't fire for a real network/RPC failure
  either (the CLI's `add` command reports the same distinction, with
  a duplicate exiting 0, same as Transmission's own RPC treats it as
  success and not an error)
- Adding an invalid magnet link, a corrupt/unreadable `.torrent`, or an
  unreachable `http(s)://` URL also shows a popup with the specific
  reason instead of doing nothing — whatever Transmission's own RPC
  reported (e.g. "invalid or corrupt torrent file", an error fetching
  the URL) if the request reached it at all, or the underlying network
  error if it couldn't even connect
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

**Settings (F9, "Settings" menu)**
- Refresh interval (seconds), host, port, RPC username/password,
  interface language — applied immediately and saved to disk (see
  "Configuration file" below for where and how)
- Global (session-wide) download/upload speed limits — read from and
  written straight to the Transmission daemon itself (`session-get` /
  `session-set`), not stored in this app's own settings file; these are
  the defaults any torrent without its own override (above) follows
- Changing the language shows a popup noting that a restart is needed
  for the menu bar and status bar to relabel — everything else already
  has (see "Internationalization" below for why those two specifically
  lag behind)

**Filters ("Columns" menu)**
- Narrows the main list to torrents matching ALL active filters (AND,
  not OR): a case-insensitive substring match on the name, and a
  multi-select checklist of every torrent status (Stopped, Queued for
  check, Checking, Queued for download, Downloading, Queued for
  seeding, Seeding) — unchecking one hides torrents in that state
- "Reset" clears the name field and re-checks every status (back to
  showing everything) without closing the dialog, so the effect is
  visible before deciding whether to confirm it
- Applied to already-fetched data (no extra RPC round-trip) and
  re-applied automatically on every periodic refresh
- Saved to disk the same way as everything else in "Settings" above,
  and restored on the next launch

**Manage columns... ("Columns" menu)**
- One window for everything about the main list's columns, replacing
  what used to be three separate entry points (a "Resize columns"
  submenu, an "Order columns" submenu, and a standalone "Columns..."
  checkbox dialog) — see "Fixed bugs" below for why
- Shown as a small grid of its own — one row per real column (16 in
  total: the 7 shown by default, plus 9 more hidden by default — see
  below), with its label, current width, and a `[X]`/`[ ]` visibility
  marker — select a row, then:
  - **Resize**: the same keyboard-driven resize (Left/Right live,
    Enter confirms, Esc cancels) that dragging a column header
    separator does
  - **Move**: the same reorder ("<"/">" markers, Left/Right, Enter/Esc)
    that double-clicking a column's header does
  - **Toggle visible**: shows or hides it immediately — hiding doesn't
    lose its width or position, it just takes no screen space until
    shown again, reappearing exactly where it was
  - **Reset**: puts width, order, AND visibility back to this list's
    built-in defaults, all at once (the 9 columns below go back to
    hidden, not shown)
- Every change applies to the actual list immediately, so there's
  nothing to separately confirm — closing the window (or the app
  exiting) is what persists the current state to `settings.json`

**The nine hidden-by-default columns**, shown from "Manage columns...":
ratio, all-time uploaded, all-time downloaded, location (download
folder), ETA, peers connected, queue position, bandwidth priority, and
completion date. Requested from the daemon on every periodic refresh
alongside the always-visible columns' own fields (not fetched lazily
only once shown), so a column shows current data the moment it's made
visible rather than needing a refresh cycle first. Special values are
shown in a human-readable way rather than as raw numbers: ratio is
"∞" for a torrent that's uploaded without downloading anything and "—"
where Transmission has no ratio to report yet; ETA is "—" when it
can't be estimated (not downloading, or not yet enough data to guess);
completion date is "—" for a torrent that hasn't finished yet; queue
position is shown 1-based (matching how you'd count it, not
Transmission's own 0-based internal numbering).

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

**Help menu**
- "About" shows the app name, version (`src/Version.h` — bumped by
  hand, not tied to any build/commit counter), copyright (current year,
  computed at runtime so it doesn't need a manual update every January),
  and the project's repository URL

**Internationalization**
- English (default), Italian, French, German, and Spanish, selectable
  from the Settings dialog via a real `TComboBox` (see "Building" and
  "Fixed bugs" below — tvision itself has no built-in combo/dropdown
  control; this project currently points at a fork that adds one,
  pending a PR upstream)
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

The main list's periodic refresh (`listTorrents()`) only requests a
lightweight set of fields — the extra detail shown in a torrent's
details window or tracker list (location, magnet link, piece info,
all-time totals, per-tracker stats, ...) is fetched with its own
separate request, made only when that window is opened (or its
"Refresh" button pressed, for trackers), so the fields most torrents'
rows don't need aren't carried on every refresh tick for every torrent.

## Configuration file

Settings (host, port, user, password, refresh interval, language, the
torrent list's last sort column/direction, column widths/order/
visibility, and the active filter) are stored in:

```
$XDG_CONFIG_HOME/tv-transmission/settings.json
```

or `~/.config/tv-transmission/settings.json` if `XDG_CONFIG_HOME` isn't
set. The file is read at startup and rewritten every time you confirm
the Settings or Filters dialog, close the "Manage columns..." window,
or click a column header to sort by it (or, from the CLI, whenever
you'd change them from the TUI — the CLI itself is read-only with
respect to this file). Column widths, order, and visibility are the one
exception to "rewritten immediately after each individual change":
resizing and reordering are both live, continuous interactions, so
writing the file on every intermediate step (every keypress or drag)
would be far more disk I/O than the final choice actually needs — all
three are instead captured and saved together once, when "Manage
columns..." closes, and again on exit as a backstop (`App::shutDown()`)
in case the app quits without that window ever being opened after the
last change.

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
   git submodule add https://github.com/zanac/tvision external/tvision
   git submodule update --init --recursive
   ```
   This points at [zanac/tvision](https://github.com/zanac/tvision), a
   fork of the real upstream ([magiblot/tvision](https://github.com/magiblot/tvision))
   that adds `TComboBox` (a proper drop-down combo box — tvision has none
   built in; see [issue #173](https://github.com/magiblot/tvision/issues/173),
   open since 2025 with no resolution). It's otherwise identical to
   upstream — no other changes, nothing removed or renamed — so this is
   purely additive and temporary: once that PR is merged upstream,
   switch this line back to `https://github.com/magiblot/tvision` and
   nothing else in this project needs to change (`Uses_TComboBox` and
   the rest of the `TComboBox`/`TComboWindow`/`TComboViewer`/
   `TComboItem` API are exactly what the real upstream will provide).
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
  Version.h                  App version string (see the "About" dialog)
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
    FilterDialog.h/.cpp         "Filters" dialog
    ColumnManagerDialog.h/.cpp  "Manage columns" window (resize/move/show/hide,
                                all in one place — built on TGridView itself)
    LanguageComboBox.h/.cpp      Compact custom combo box for the language picker
    WindowListDialog.h/.cpp     "Window list" dialog
    AboutDialog.h/.cpp          "About" dialog
    BandwidthStatusLine.h/.cpp  Status bar with a runtime-updatable item
packaging/
  appimage/
    build-appimage.sh          Builds build/TvTransmission-x86_64.AppImage
    tv-transmission.desktop     Desktop entry (Terminal=true)
    tv-transmission.png         Placeholder icon
  tgridview/
    TGridView.h/.cpp           Generic dynamic-column list view — see its own
                                README.md; self-contained, no dependency on
                                the rest of this project, meant to be copied
                                into other Turbo Vision projects wholesale.
                                Used by TorrentListWindow (ui/) for the main
                                list's rendering.
    TGridWindow.h/.cpp          Optional TWindow wrapper (fullscreen-locked or
                                ordinary MDI child) hosting one TGridView —
                                TorrentListWindow (ui/) derives from this
    README.md                   Full design writeup for this standalone module
```

## Known limitations

- The magnet link shown in the details window is truncated, as a visual
  reference only — a TUI has no clipboard integration, so there's no
  "copy" action for it; select it with your terminal emulator's own
  text selection if you need the full value (which a truncated line
  doesn't help with either way).
- The torrent list's progress bar uses Unicode block characters
  (`█`/`░`), which need a UTF-8-capable terminal to render correctly —
  chosen deliberately over an ASCII-only bar for a nicer look, on the
  assumption that's the overwhelmingly common case today.
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

**Moved "Filters..." and "Manage columns..." into their own "Columns"
menu**, out of "Settings" (which now holds just the "Settings..." item
it started with). Both were genuinely about the torrent list's columns
and filtering, not application settings — grouping them together
reads more clearly than having them share a menu with an unrelated
"Settings" entry just because that's where they first landed.

**`TGridView`'s header is now 2 rows tall: column labels, then a full
"=" rule line**, separating the header from the actual data rows below
— not a fill *next to* the last column, which is what the first attempt
at this did before being corrected. Clicks landing on the rule row
(rather than the label row above it) are explicitly ignored, so the
sort glyph and reorder markers can't be accidentally triggered by a
click that only shares the same x position on the wrong row. Verified
by actually capturing a rendered screen (a real pty, not just reading
the draw code) — both a standalone grid and this project's own main
torrent list — confirming the rule line sits between the header and the
data rows in both, and that a simulated click on the rule row does
nothing while the same x on the label row above it still works.

**A column header's double-click (to reorder) and single-click (to
sort) were colliding.** A double-click arrives as two separate
mouse-down events — an ordinary one first, then a second one carrying
the double-click flag — so the *first* click of an intended
double-click was indistinguishable from a plain single click at the
moment it happened, and fired the sort-toggle immediately; the second
click then *also* started a reorder. Fixed by giving sorting its own
dedicated, single-character hotspot — a "□" glyph (turning into "^"/"v"
once that column is the active sort) reserved at the last character of
a sortable column's width, checked first and unconditionally regardless
of the double-click flag, so clicking it is sort-only on either half of
a double-click and never starts a reorder. A plain click anywhere else
on a column's name now does nothing on its own (only a double-click
there starts a reorder), removing the collision entirely. Verified
directly: a click on the glyph's exact position sorts; a click
elsewhere on the name does not; the *first* mouse-down of a simulated
double-click (before the flag would even be known) doesn't sort either;
and a double-click landing squarely on the glyph itself still only
sorts — it never starts a reorder, matching the two being fully
separate hotspots now.

**The reorder mode's right-hand "▷" marker sometimes wasn't clickable.**
Root cause: the marker text was built by concatenating "<"/the label/
the sort indicator/">" into one string and letting `fitToWidth()`
truncate whatever didn't fit within the column's width — which silently
dropped the trailing ">" (rarely the leading "<", since it isn't at the
truncated end) whenever a column's label came close to filling its own
width, exactly the case a longer torrent-list column name would hit
more often than not. The hit-test, meanwhile, still expected it at a
fixed position regardless — clicking there hit nothing, because nothing
was actually drawn there. Fixed by reserving "<"/">" (and the sort
glyph above) at their exact intended positions explicitly, rather than
via concatenation-then-truncation, so what's drawn and what's
hit-tested can never drift apart. Verified with a column name long
enough to have triggered the old truncation: a simulated mouse click at
the marker's fixed position now reliably moves the column, every time.

**"Manage columns..." now toggles a row's visibility on double-click**,
not only via the "Toggle visible" button — the same
`setRowActivateCallback()` mechanism (double-click / Enter) `TGridView`
already provides elsewhere in this app (e.g. opening torrent details),
wired to the identical toggle logic the button already used rather than
a second copy of it. Verified directly: double-clicking a row hides it,
and double-clicking it again shows it back — a real toggle, not a
one-way action.

**"Manage columns..." wasn't visibly highlighting the focused row.**
Its meta-grid never set a row-color callback, so it fell back to
`TGridView`'s own default (`TListViewer`'s inherited palette colors 1
and 2 for normal/focused) — functionally correct (clicking a row *did*
move focus, as confirmed directly), but those two colors don't contrast
enough inside a `TDialog` to actually notice which row is selected.
Fixed the same way the main torrent list already handles this: an
explicit `setRowColorCallback()` returning a fixed black-on-white for
the focused row regardless of the dialog's own palette.

**Added nine optional, hidden-by-default columns** (ratio, all-time
uploaded/downloaded, location, ETA, peers, queue position, priority,
completion date), all fetched by `listTorrents()` itself on every
periodic refresh — not lazily the first time a column is shown — so
toggling one visible in "Manage columns..." shows current data
immediately rather than needing a refresh cycle to catch up. Extended
`SortColumn` with nine more values *after* the original seven rather
than interleaving them, specifically so a `sortColumn` value saved by
an older version of this app still means the same column after
upgrading. `TorrentListWindow::columnVisibility()`/
`setColumnVisibility()`'s fallback for a missing entry had to stop
defaulting to "shown" unconditionally once this landed: a
`settings.json` saved before these nine existed only has 7 entries, and
treating every missing one as "shown" would have made all nine appear
visible by default for anyone upgrading, the opposite of what "hidden
by default" is supposed to mean — fixed by defaulting index 0-6 to
shown and 7-15 to hidden specifically, rather than one blanket default
for every index. Also hit a real naming collision while wiring the new
header labels: `Str::HeaderDownloaded` already existed, for the
tracker-list window's own "Downloaded" column — renamed the new torrent
-list ones to `HeaderTotalUploaded`/`HeaderTotalDownloaded`, which also
reads more clearly next to the already-existing rate-based `HeaderUpload`/
`HeaderDownload`. Verified end to end against a mock server for both
common and edge-case values: the RPC request itself actually asks for
all nine new fields; the default visibility split (7 shown, 9 hidden)
is exactly right, including from an old 7-entry settings file; and the
special-value formatting for ratio (∞ for an uploaded-only torrent),
ETA (unknown), priority, and completion date (not yet finished) each
render as the intended human-readable text rather than a raw sentinel
number.

**Rationalized what had grown into three separate column-management
entry points** (a "Resize columns" submenu, an "Order columns"
submenu, and a standalone "Columns..." checkbox dialog — all added
incrementally, each solving one problem at a time) **into a single
"Manage columns..." window.** Deliberately reused rather than
reimplemented the existing interactive primitives: the window's
"Resize" and "Move" buttons call straight into `TorrentListWindow::
startColumnResize()`/`startColumnReorder()` — the same blocking Left/
Right/Enter/Esc loops already used by the header's own mouse and
keyboard entry points — so there was no new interactive-loop code to
get wrong here, only new code to pick *which* column those loops act
on. That "which column" picker turned into a small grid of its own —
one row per real column, showing its label/width/visibility — built
with `TGridView` on itself (not resizable or reorderable itself, since
reordering a list *of columns* would be a strange thing to offer): a
reasonable proof that the generic widget holds up being used this way,
not just for a torrent list. Added `TorrentListWindow::
resetColumnLayout()` for the window's own "Reset" button, since
resetting width+order+visibility together wasn't something any
existing method did — it re-adds all 7 columns fresh (their own default
widths) and re-shows every one, rather than needing three separate
reset calls. Verified directly: the meta-grid actually reflects a real
`TorrentListWindow`'s current label/width/visibility per row; the
"Toggle visible" button changes the real list's visibility, not just
something local to the dialog; and "Reset" restores all three
properties (widths, order, *and* visibility) in one call rather than
requiring separate cleanup.

**Added showing/hiding individual columns** (Settings → Columns..., a
checkbox per column plus Reset), which pushed `TGridView`'s visual/
logical index split (see the column-reordering entry below) one step
further: every visual operation already had to translate a display
*position* to a column's stable logical identity for reordering to work
correctly — hiding needed those same operations to also skip a column
entirely, as if it took zero screen space, while a caller's own
`cellText()`/`cellBold()`/etc. callbacks keep referring to it by that
same stable identity whenever they're asked about a column that's
merely somewhere else, never one that no longer exists. Solved by
having `visibleDisplayOrder()` — a fresh filter over `columnOrder()`
down to just the shown columns, computed wherever it's needed — become
the one sequence every layout, drawing, and hit-testing path iterates,
instead of the raw (possibly-including-hidden) order. The trickiest
part was making that consistent with reordering *while* some columns
are hidden: swapping two positions that are visual neighbors doesn't
mean their positions in the underlying (unfiltered) order are adjacent,
if a hidden column happens to sit between them — `runReorderLoop()`
resolves each visible neighbor's *true* position before swapping, so a
column can be moved past a hidden one exactly as if it weren't there,
without disturbing where the hidden one itself will reappear once shown
again. Verified directly: a hidden column is skipped entirely by
rendering (its `cellText()` callback never even gets called for it);
showing it again restores its previously-remembered position; and —
the scenario most likely to have a subtle bug — reordering two visible
columns with a hidden one sandwiched between them swaps the right
pair, skips over the hidden one correctly, and leaves every column
accounted for (no duplicates, none dropped) once the hidden one is
shown again. Also verified end to end against a mock Transmission
server: hiding columns at startup excludes them from the actually-
rendered row, and showing them again at runtime brings them back.

**Added column reordering** — double-click a header (or, from the
keyboard, Settings → Order columns) to move a column left/right,
Enter/a non-marker click confirms, Esc restores the order it had when
the mode was entered. This required a real structural change to
`TGridView`, not just another interactive loop bolted on next to
resizing: every column index the widget hands to a caller's own
callbacks (`cellText`, `cellBold`, the sort/resize APIs, ...) had to
keep meaning the same *logical* column — its stable identity, assigned
once when it's added — regardless of where it's currently *drawn*.
Before reordering existed those two concepts were the same number, so
nothing distinguished them; letting the user rearrange columns broke
that assumption; a caller's `cellText(row, col)` switching on `col` to
decide which field to return would otherwise start returning the wrong
field for whatever moved. Fixed by adding a `displayOrder_` permutation
(visual position → logical index) that the header and rows both
translate through on every draw and every hit-test, while `column()`
and every callback keep using logical indices exactly as before —
`TorrentListWindow`'s own callbacks didn't need a single line changed.
Wiring the mouse and keyboard entry points into one shared interactive
loop (`runReorderLoop()`) reused the same `TView::getEvent()` primitive
`startKeyboardResize()` already relied on, this time pumping and
handling both key and mouse events in the same loop rather than one or
the other. Verified directly: the logical-index guarantee itself (the
whole point of the change) — a caller's callback receives the correct
logical index throughout a rearranged order; `setColumnOrder()`
rejecting anything that isn't exactly a permutation (wrong size, an
out-of-range or duplicate index); a real reorder sequence via injected
events (Right, Right, Enter confirming the expected final order, and
separately Esc reverting with no callback firing); and, end to end
against a mock Transmission server, that a torrent's data stays
correctly associated with each column after reordering (moving the
Status column first, the rendered row genuinely shows the status text
before the name, not scrambled data). Reused the same nested-submenu
`(TMenuItem&)` cast idiom (see the "Resize columns" entry below) for
the new "Order columns" submenu.

**Added keyboard-driven column resizing** (`TGridView::
startKeyboardResize()`), alongside the existing mouse-drag one — the
same live-feedback resize, just started from a menu instead of a mouse
grab, for when a mouse isn't convenient. Uses the same underlying
primitive the mouse-drag already relied on: `TView::getEvent()`, which
`mouseEvent()` itself is built on internally (see `tview.cpp`) — any
view can call it to synchronously pull the next event, which is what
makes this callable from outside the header's own event handling (a
menu command), not just from within a mouse-down already being
processed there. Left/Right adjust the width live (redrawn after every
press), Enter confirms, Esc restores the width the column had when the
mode was entered. Guards against a menu item invoking this on an
out-of-range or non-resizable column by returning immediately, before
ever entering the event-pump loop — verified directly (with a timeout
wrapped around the test, specifically to catch a regression that made
it loop forever instead of returning): both cases return without
blocking. The interactive part itself — actually pressing arrows and
watching the column follow — needs a real terminal to verify, the same
limitation as the mouse-drag resize and any other interactive input
built on tvision.

Wiring this into a menu (Settings → Resize columns, listing each
column) surfaced a real gotcha in how tvision's menu-building operators
work: `TSubMenu`'s `operator+(TSubMenu&, TSubMenu&)` chains the second
submenu as a **sibling** at the same menu level, not nested inside the
first one's dropdown — only `operator+(TSubMenu&, TMenuItem&)` nests,
and overload resolution picks the sibling-chaining one whenever the
right-hand side is *statically* a `TSubMenu`, which a nested submenu
naturally is. Without realizing this, "Resize columns" would have
appeared as a new top-level menu bar entry (a sibling of "Torrent",
"Settings", ...) instead of living inside "Settings" as intended, and
the "Settings" item itself would have ended up nested *inside* "Resize
columns" by the same mechanism. Fixed the same way tvision's own
`tvedit`/`tvdemo` examples do it: wrap the whole nested submenu
subexpression in an explicit `(TMenuItem&)` cast before combining it
into the outer chain, forcing the nesting overload instead of the
sibling one.

**Sorting-on-click moved from this app into `TGridView` itself, to
actually be the reusable behavior the widget's own README claims.**
After the migration (see the entry below), clicking a header still
worked, but the "click toggles ascending/descending" and "draw a `^`/`v`
indicator on the active column" logic lived entirely in
`TorrentListWindow` — meaning another project reusing `TGridView` would
have to reimplement both from scratch, and the indicator was appended
directly into `TGridColumn::header`'s stored text, so every language
switch had to remember to strip the old one before applying a new
label. Moved into `TGridView`: it now tracks which column is sorted and
in which direction, toggles that itself on a click to a `sortable`
column (new per-column flag), and draws the indicator at draw time
without ever touching the stored header text. `TGridView` still has no
idea how to reorder rows (consistent with owning no row data at all —
see its README's core design principle), so a `SortChangedFn` callback
tells the owner which column and direction were chosen; a separate
`setSortIndicator()` lets the owner restore a persisted sort at startup
without that callback firing (there's nothing to react to — the data is
about to be loaded already in that order). Verified with simulated
header clicks (not just reading the code): clicking an unsorted column
selects it ascending and fires the callback, clicking the already-
active column flips to descending, and `setSortIndicator()` sets the
displayed state without firing anything.

**Torrent-list column widths are now saved and restored**, the same way
everything else in `settings.json` is, with one deliberate difference:
instead of writing the file on every resize (a live, continuous drag —
see `TGridView`'s own resizing design), widths are captured once at
exit, via overriding `App::shutDown()` (already called from `main.cpp`
right after the run loop returns) to read `TorrentListWindow::
columnWidths()` and save. Applied back through a new constructor
parameter, matched against the current column count so a first run (no
saved widths yet) or a `settings.json` from a version with a different
number of columns falls back cleanly to the built-in defaults rather
than applying a mismatched, out-of-order list. Verified: the full
save/reload round trip through `settings.json` (including the no-
`columnWidths`-key backward-compatibility case), and that a
`TorrentListWindow` actually applies persisted widths at construction —
correctly falling back to defaults for both the empty-vector (first
run) and wrong-count (stale settings file) cases.

**New: `TGridView`, a generic dynamic-column list widget** (`src/tgridview/`),
extracted from this project's own fixed-column torrent list rendering
so it can be reused in other Turbo Vision projects — see its own
`README.md` for the full design writeup (why the data source is
callback-based rather than the grid owning a copy of the rows, exactly
how mouse-driven column resizing behaves and why it resizes only the
column being dragged rather than redistributing space across others,
and the two ways to host it — a plain `TView` insertable into any
window, or the optional `TGridWindow` convenience wrapper with a
`fullScreen` flag mirroring this project's own locked-fullscreen vs.
ordinary-MDI-window distinction). Verified directly: dynamic column
add/insert/remove/clear at runtime, cell text read live through the
callback (changing the underlying data changes what's shown on the
next draw, with nothing to keep in sync by hand), column width
clamping to each column's own minimum, and the double-click row-
activation broadcast. The interactive part of mouse-driven resizing —
actually dragging a column border and watching it follow the mouse —
needs a real terminal to verify, the same limitation that applies to
any interactive mouse handling built on tvision.

**`TorrentListWindow` migrated to use it**, replacing its own
purpose-built column/header/row rendering (`TorrentListViewer`,
`TorrentListHeader` — both gone now) with `TGridView` underneath.
Wiring this up surfaced two real gaps in `TGridView` itself, fixed as
part of the migration rather than worked around in the app: (1) there
was no way to know when the focused row changed via arrow keys or a
plain click — only double-click (`RowActivateFn`) and right-click
(`RowContextFn`) had callbacks — which this app needs to enable/disable
the Start/Stop/Remove/... commands for whichever torrent is currently
selected; added `setRowFocusCallback()`. (2) the row-color callback was
only ever invoked for *non*-focused rows, silently falling back to
`TListViewer`'s own default "selected" look for the focused one — fine
for a grid with no opinion on that, wrong for this app, which wants its
own black-on-white focused-row look applied consistently *and* the
bold-name styling to keep applying even when a row is focused (both of
which the original hand-rolled rendering already did). Fixed by always
invoking the row-color callback with the correct `focused` flag, and no
longer skipping the bold-cell callback for the focused row. Verified
end to end against a mock Transmission server (not just against fake
in-memory data): the real per-column data renders correctly (name,
progress bar, size, rates, status), the default name-ascending sort is
correct, a name filter narrows the grid and the bandwidth totals only
count what's actually visible, and — importantly, since this is
exactly the kind of thing a subtle row/index mixup could get wrong —
`startSelected()` acts on the *same* torrent that's actually focused
after sorting, not on a stale index. One cosmetic side effect worth
knowing about: the Download and Upload columns are now genuinely
separate columns (matching `SortColumn`'s own enum, which already
treated them as such for sorting) with a normal separator between them,
where before they were packed together with no gap; and the list picked
up mouse-driven column resizing (`gvResizableColumns`) for free, except
on the progress-bar column, which stays fixed-width since the bar
itself is always rendered at a fixed size.

**Added torrent filtering (name substring + status multi-select) and a
dedicated "Settings" menu.** The Settings item moved out of the Torrent
menu into its own top-level menu, with a new "Filters..." item above
it. The harder part was integrating filtering into `TorrentListViewer`
without disturbing anything already built on top of "the list of
torrents": every method that used to read `torrents_` directly
(`getText()`, `draw()`, `selectedTorrent()`, the two rate totals) had to
switch to a separate `visible_` vector, computed by filtering then
sorting `allTorrents_` (every torrent last fetched, untouched) —
otherwise changing the filter would need a fresh RPC round-trip just to
recompute what should be visible, or would corrupt the "full" data with
whatever was hidden. `setFilter()` re-derives `visible_` from the
already-held `allTorrents_`, matching the same reasoning already used
for `getTorrentDetails()`/`getTrackerStats()` being separate on-demand
calls elsewhere in this project — apply what's already fetched instead
of re-fetching by default. Verified directly: a name filter matches
case-insensitively regardless of the query's own case, a status filter
correctly narrows to just the checked states, combining both is a real
AND (not OR — a torrent matching only one of the two stays hidden), the
rate totals only sum currently-visible torrents, and resetting the
filter brings every torrent back. Filter settings round-trip through
`settings.json` correctly, and a settings file saved by a version
without this feature (no `"filter"` key at all) still loads with the
correct "show everything" default instead of failing or hiding
everything.

**A real `TComboBox` is now available**, via the `zanac/tvision` fork
this project points at (see "Building" above) — built to the exact
design already settled on for it: a popup styled like the existing
`THistoryWindow`/`THistoryViewer` pair (not a menu-style overlay), and
entries (`TComboItem`) with a displayed label plus an opaque `value`
the caller can use to identify the choice. Verified directly
(construction with an initial focus, `focusItem()` including
out-of-range clamping, `newList()` replacing the item chain and
updating the focused value, destruction freeing the chain without
leaking or crashing, and the empty-list case) against the actual
library build produced by this project's own CMake — not just read
from source.

`LanguageComboBox` (previously a hand-built `TView` that opened a
`TMenuPopup` — a generic-string, id-based design, unlike `TComboItem`'s
label+value) has since been switched over to it: it's now a thin
`TComboBox` subclass that only supplies the 5-language `TComboItem`
chain and a typed `language()` accessor (`TComboBox::value` cast back
to `Language` — exact, since every item's `value` was built from a
`Language` enumerator to begin with), while `TComboBox` itself handles
drawing, opening its own popup, and keyboard/mouse input. Verified
end-to-end: construction with each of the 5 languages as the initial
value, `focusItem()` updating `language()` correctly, and the full
`SettingsDialog` flow (`createSettingsDialog()` pre-filling the combo,
`settingsDialogResult()` reading back a language chosen through it).

**Added French, German, and Spanish, with a compact combo box instead
of radio buttons for picking a language.** Every one of the ~130
translatable strings needed a third/fourth/fifth variant, not just a
new set — the old `tr()` used a plain English/Italian ternary
throughout, so this meant replacing it with a 5-way `pick(en, it, fr,
de, es)` helper and updating every call site, all in one pass rather
than piecemeal (an exhaustive `switch` over `Str` was already the
existing convention specifically so a missed/misordered translation
shows up on review instead of silently). For the combo box: tvision has
no built-in combo/dropdown widget at all (checked: nothing in
`dialogs.h`/`views.h` resembles one) — `LanguageComboBox` is a small
custom `TView` that shows the current language name on one line and
opens a `TMenuPopup` (the same mechanism behind the torrent list's
right-click menu) listing all 5 below it, replacing what used to be a
2-row `TRadioButtons` cluster with 1 row. Verified: `tr()` returns
distinct, correct text for a representative sample of strings (menu
labels, the CLI's help text) in all 5 languages, including that the
native language names themselves don't change with the current
language; `LanguageComboBox` constructs correctly and reports back
whichever language it was given for all 5; and the full save/reload
round trip through `settings.json` preserves all 5 language values.

**Adding an already-added torrent did nothing, with no indication why.**
`torrent-add`'s response was only ever checked for `"result":"success"`,
which Transmission returns whether the torrent was genuinely new or
already present — the two cases are distinguished by which key shows up
under `arguments` instead (`"torrent-added"` vs `"torrent-duplicate"`),
which nothing was checking. Fixed by having `TransmissionClient::
addTorrent()` return which of the two (or failure) actually happened,
and showing a `messageBox` for the duplicate case in the TUI (the CLI's
`add` command reports it as text instead, still exiting 0 — matching
that Transmission itself doesn't treat this as an error). The failure
case had the exact same problem, just unnoticed until asked about
directly: an invalid magnet, corrupt `.torrent`, or unreachable URL also
did nothing, silently. Fixed the same way — `addTorrent()` now also
captures Transmission's own error text for that request (its `"result"`
field, when not `"success"`) into `lastError()`, distinct from a
network/curl-level failure that never reaches Transmission at all
(`lastError()` already carried that from `call()`, just wasn't being
shown anywhere for this specific action). Verified against a mock
server for all four outcomes: added, duplicate, Transmission's own
rejection (with its exact error text asserted, not just "some error"),
and a fully unreachable server.

**Settings could silently wipe out real global speed limits on first
setup.** The Settings dialog fetches the server's current global speed
limits (`getSessionLimits()`) *before* opening, using whatever
connection is active at that point, then writes back whatever the
dialog shows when confirmed. If that initial fetch failed — most
commonly the very first time host/user/password are being configured,
since the *old* connection can't reach anything yet — it silently
returned an all-zero/disabled `SessionLimits`, indistinguishable from
"the server genuinely has no limits set". Confirming the dialog then
applied the *new*, now-working connection settings first, and only
after that sent those meaningless zeros back — to a connection that,
by then, actually worked, potentially overwriting real limits already
configured on that server, even though the user only meant to set up
the connection and never touched the speed-limit fields. Fixed by
having `getSessionLimits()` report whether the fetch actually succeeded
(an optional `bool*` out-parameter), and only sending values back to
the server when it did. Verified directly: against an unreachable
server the flag comes back false and the (still zero) values are
correctly recognized as not real; against a real server, the flag is
true and the actual configured values come through.

**"Add torrent" gained a "Browse..." button**, first as a button inside
the "Add torrent" dialog that opened tvision's own `TFileDialog`
(filtered to `*.torrent`) on top of it. That didn't last: nested inside
this app's already-modal "Add torrent" dialog, `TFileDialog` rendered
with wrong colors and garbled text (fragments of both the underlying
torrent list and the file dialog itself, bleeding into each other) —
first suspected as a `TFileDialog`-specific quirk, since even tvision's
own `tvedit` example never nests a dialog this way (it opens
`TFileDialog` directly from the application, one level deep). Replaced
with a small custom directory browser (`TListViewer`/`TStaticText`/
`TButton`, the same building blocks as everything else in this app),
expecting that to sidestep whatever was `TFileDialog`-specific about
it. It didn't: the *exact same* garbling showed up with this hand-built
dialog too, which ruled out either widget's own implementation as the
cause. The real common factor was the nesting depth itself — two modal
dialogs deep, on top of this app's fully custom `TorrentListViewer::
draw()` — regardless of which widget sat in the inner slot. With that
understood, the fix didn't need a custom browser at all: "Browse" now
destroys the "Add torrent" dialog *first*, opens `TFileDialog` directly
from the application (one level deep, exactly like every other dialog
here), and — if a file was picked — reopens "Add torrent" with that
path pre-filled (`createAddTorrentDialog()` takes an optional initial
value now) so the user still sees/can edit it before confirming.
`TFileDialog` never ends up nested inside another modal dialog, so the
rendering bug's actual trigger never occurs; the custom browser this
went through along the way was reverted, since it no longer served a
purpose once the real cause was found. Verified: `createAddTorrentDialog()`
pre-fills its field correctly both with and without an initial value.
The nesting-depth fix itself (that `TFileDialog` no longer renders
wrong) could only be confirmed by inspection and running the real app,
since reproducing the original rendering artifact needs an actual
terminal.

**Torrent details window, extended with location/privacy/magnet link/
piece info/all-time totals/ratio/activity/elapsed-time fields**,
matching what a reference Android Transmission client shows. Fetched
via a new `TransmissionClient::getTorrentDetails()` — deliberately
separate from `listTorrents()` (same reasoning as `getTrackerStats()`):
these fields aren't needed until the user opens details for one
specific torrent, so the periodic list refresh stays lightweight rather
than carrying them for every torrent on every tick. "Availability %" in
particular has no direct field in Transmission's RPC; it's computed
with the same formula Transmission's own official GTK/Qt clients use:
`(haveValid + haveUnchecked + desiredAvailable) / sizeWhenDone * 100`.
The first version of this laid every field out one per line, which grew
the window to 41 rows tall — taller than most terminals, so it got cut
off rather than actually showing everything. Fixed by pairing two short
fields per row wherever they're naturally related (size + piece info,
completion % + availability %, added date + last activity, etc.) and
dropping the "Transfer:"/"Activity:"/"Time elapsed:" section headers
entirely (each field's own label is already clear without one), bringing
it down to 27 rows for the same content. Still too tall in practice — a
screenshot showed the window taking up nearly the entire terminal, and
the buttons looking stuck directly to the speed-limit checkboxes with
no visible gap. The second problem turned out to be a real bug, not
just a spacing choice: tvision's `TCluster::drawMultiBox()` (the code
behind `TCheckBoxes`) loops `i <= size.y` rather than `i < size.y`, so
it paints one extra row right below its declared bounds — blank of
text, but still filled with its own highlighted background color. That
phantom colored row was exactly the blank margin this window left
before its buttons, so the row was never actually blank on screen. Not
something fixable in the vendored library from here; worked around by
reserving two blank rows before the buttons instead of one, so at least
one of them is guaranteed to be genuinely blank regardless of the
overdraw. Further compaction (moving the ID field to share a row with
Privacy instead of sitting alone, dropping another now-redundant blank
separator) brought the window down to 25 rows. That 25 was still a
fixed height sized for a fairly full case, though — a torrent with
fewer optional fields (no error, sometimes no location) left visible
empty space below the buttons, per a follow-up screenshot. Fixed by
computing the height from what a given torrent's own fields actually
need instead of a fixed guess: a first pass builds the list of rows to
show (without creating the window yet, since its height depends on the
result) and only then is the window created at exactly that height,
followed by a second pass that actually inserts the views. Verified
directly: two torrents with the same set of optional fields present
produce windows of the identical height, adding an error string adds
exactly one row, and a minimal torrent (missing most optional fields)
produces a noticeably shorter window rather than reusing the fixed one.

**Torrent list rendering, rewritten for a bolder name and per-status
color.** Previously the list relied on `TListViewer`'s inherited
`draw()`, which paints an entire row with a single `TColorAttr` from
one `getColor()`/`mapColor()` call (that's how the earlier "always
blue" look was done). A bold name plus a status-dependent color within
the *same* row isn't expressible that way — both the color and the
weight need to vary within one row, not just between focused and
unfocused. Fixed by overriding `draw()` completely: each row is now
built as several `TDrawBuffer` segments (name, progress bar, size,
rates, added, status), each with its own `TColorAttr`, placed one after
another by tracking the column position (`TDrawBuffer::moveStr()`
writes at an absolute column and returns the width consumed). The old
`mapColor()` override is gone — nothing calls it anymore now that
`draw()` doesn't go through the inherited path.

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
