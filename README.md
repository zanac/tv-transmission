# TV Transmission

**Version 1.5.2** — stable release.

A terminal UI (and CLI) client for Transmission (`transmission-daemon`),
built on [Turbo Vision (magiblot/tvision)](https://github.com/magiblot/tvision),
written in C++17.

It talks to `transmission-daemon` over its JSON RPC (HTTP, port 9091 by
default), so no native Transmission library is needed — just libcurl for
HTTP and nlohmann/json for parsing.

## Features

**Torrent list — one window per configured server**
- Every server named in the Connection dialog (see below) gets its own
  torrent-list window, all open at once, each always exactly filling
  the whole desktop (adjusting automatically if the terminal itself is
  resized) and stacked on top of each other rather than side by side —
  titled with that server's own logical name ("home — Torrents", say)
  so whichever one is on top is easy to identify. Deliberately NOT
  closable, movable, or resizable from the window itself (no close box,
  Alt+F3 does nothing, dragging or Ctrl+F5-zooming doesn't either) —
  every configured server is meant to always have its own window open,
  always at that one size; the only way to actually remove one is the
  Connection dialog's own "[-]" on that server's name (see below),
  which closes its window along with removing it (see "Fixed bugs"
  below for the two back-and-forth changes this went through — an
  ordinary resizable/tileable MDI window in between, briefly — before
  settling back on always-maximized for good reason)
- **Connections** (its own menu) lists every configured server by
  name, a bullet and the whole name highlighted for whichever one is
  currently on top — pick a different one to bring its own window
  forward instead, or check the menu at a glance to see which one
  that already is without needing to bring anything forward first. No
  servers configured yet shows a single, disabled "Empty" entry instead
- **Window → Tile/Cascade** only ever affects this app's own other
  window types (Torrent details, Files, Tracker) — every torrent-list
  window being always exactly the same size (the whole desktop) makes
  tiling or cascading them against each other meaningless; **Connections**
  above is what actually switches between them. **Window → Window
  list...** (Alt+0) still lists every open window of every kind
  together, including torrent-list ones, and still jumps straight to
  whichever is picked
- Every Torrent-menu action (Start, Stop, Queue, Select Multiple, Add,
  ...) acts on whichever torrent-list window currently has focus — the
  same "act on the focused one" rule "Manage columns..." already
  followed for whichever grid had focus, now covering every torrent
  action too, since there's more than one list to choose from
- Each window's own position and size is saved when the app closes and
  restored on the next launch (clamped to the current terminal size if
  it's shrunk since — see "Fixed bugs" below), along with which one had
  focus, so that one comes back on top of the others
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
  Width, order, and visibility are each server's own — a window resized
  or reconfigured this way doesn't affect any other open one — and
  saved/restored per server across launches (see "Fixed bugs" below for
  why this changed from a single shared layout)
- Click a column header to sort by it; click again to reverse the
  direction (a `^`/`v` indicator shows the active column and direction);
  the chosen column and direction — unlike width/order/visibility above,
  still one shared setting across every open window, not per server —
  are saved and restored on the next launch
- If enough columns are shown at once that they don't all fit — a wide
  terminal helps, but with several of the optional columns turned on
  it's easy to exceed even that — a horizontal scrollbar appears below
  the list to reach the rest, keeping the header lined up with whatever
  the rows have scrolled to. Hidden entirely (its row handed back to
  the list itself, one more visible row of torrents) whenever
  everything already fits, rather than always taking up a row for a
  control that would have nothing to do
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

**Multiple selection**
- "Torrent → Select Multiple" turns on a leftmost `[X]`/`[ ]` checkbox
  column on the main list — the same menu entry turns it back off
  again. The row that had focus when you turned it on starts checked
- Click any row (not just the checkbox itself) to check or uncheck it;
  Space does the same for whichever row is focused, so the whole thing
  works from the keyboard alone once it's on
- Double-click a row to turn selection mode on directly from there,
  with that row already checked; double-click again (on any row) to
  turn it back off — a mouse shortcut for the same thing the menu entry
  does, matching "start with one gesture, stop with the same gesture"
  (see "Fixed bugs" below for why this replaced an earlier press-and-
  hold version of the same shortcut)
- Esc or Enter turns it back off, same as the menu entry — as does
  "Cancel selection" in the right-click context menu, which only shows
  up there while there's a selection to cancel
- Once at least one row is checked, every Torrent-menu action (Start,
  Stop, Remove, Delete with files, Start Now, Verify, Reannounce,
  Details, Files) applies to every checked torrent instead of just the
  focused one — Remove and Delete ask for confirmation once, naming how
  many torrents rather than listing each one; Details and Files open
  one window per checked torrent (reusing an already-open one for a
  given torrent instead of duplicating it, same as with a single
  selection). The list stops auto-refreshing while you're selecting, so
  which row is which doesn't shift under you — it resumes, and the
  checkboxes go away, the moment an action runs or you turn selection
  off yourself
- The checkbox column stays put at the left even if the list is
  scrolled horizontally (see "Column resizing, reordering, and
  visibility" above) — it's not one of the regular columns and doesn't
  move with them

**Queue reordering**
- Transmission processes queued (not-yet-active) torrents in order —
  "Torrent → Queue" (also on the right-click context menu) gives the
  four standard moves: to the top, up one, down one, to the bottom.
  With more than one torrent checked (see "Multiple selection" above),
  it applies to all of them
- Double-clicking a torrent's own position in the (normally hidden —
  see "Column resizing, reordering, and visibility" above) queue
  column cycles through those same four actions, one per double-click:
  top, then up, then down, then bottom, then back to top — a quick way
  to reach for whichever's needed without opening the submenu each
  time, for that one torrent

**Trackers and peers**
- A "Trackers..." button in the torrent details window opens a
  separate, non-modal window with **two tabs — Trackers and Peers —
  switched via a two-item radio-button cluster at the top** (Turbo
  Vision has no tab control of its own; a native radio-button cluster,
  with its own marker glyph and keyboard navigation already built in,
  reads clearly enough as "choose one of these two" without needing one
  — see "Fixed bugs" below for an earlier, hand-built button-based
  attempt at this that didn't hold up). Both tabs are built on
  `TGridView`, the same generic widget the main list and the files
  window use
- The radio cluster's own background, and each tab's own column
  headers, both use the same blue as the rows beneath them, rather than
  the plain palette-resolved color every other `TGridView`-based
  window's own headers still use by default — see "Fixed bugs" below
- **Trackers**: host, tier, seeders, leechers, downloaded count, and a
  short status (OK/Error) for every tracker on this torrent
- **Peers**: address (`ip:port`), client name, download progress,
  current down/up speed, and Transmission's own compact status flags
  for every peer currently connected
- Columns, on either tab, can be resized, reordered, and shown/hidden —
  through the same single, focus-aware "Manage columns..." menu entry
  the main list uses, automatically once this
  window has focus; there's no button of its own for it here. Rows
  aren't sortable by a column click on either tab, for related but
  different reasons: Transmission already returns trackers in tier
  order, which is the order that matters, so a click-to-sort there
  would work against that; peers have no stable order to begin with —
  the list can reorder itself on every single refresh regardless of
  anything sorting would do — so a click-to-sort there would just as
  easily undo itself moments later. Either way, that's independent of
  the columns' own width/position/visibility, which "Manage
  columns..." still offers on both tabs
- Each tab's own column layout is saved and restored across launches
  separately — see "Configuration file" below. Both are shared, the
  same as the main list's own: not one set of tracker columns (or peer
  columns) per torrent, opening either tab for a different torrent
  later still starts from whatever was last saved or changed there.
  Switching between the two tabs within one already-open window
  remembers each one's own layout too, even before anything's been
  explicitly saved via "Manage columns..." — resizing Peers, checking
  Trackers, then coming back to Peers doesn't lose what changed
- Neither tab's own data (`trackerStats`/`peers`, both part of
  `torrent-get`) is fetched as part of the regular list refresh; each
  is requested only for whichever tab is actually showing — switching
  tabs fetches the newly-shown one fresh, the other stays as it last
  was until switched back to — and again only when this window's own
  "Refresh" button is pressed (no auto-refresh on either tab)
- Double-click a tracker row (Trackers tab only) for a small window
  with that tracker's full status: last/next announce time and the
  complete error or success message, which don't fit in a table row.
  A double-click on a Peers row does nothing — there's no equivalent
  extra detail to show for a peer that wouldn't already fit in its own
  row

**Per-file selection ("Files" — Torrent menu, or the right-click context menu)**
- A separate, non-modal window listing every file within a torrent —
  name, size, download progress, whether it's wanted, and its priority
  (Low/Normal/High) — built on `TGridView`, the same generic widget the
  main list uses
- Files inside subfolders are shown as a real tree, indented under a
  folder row for each directory level (however deep) — not a flat list
  of full paths. A torrent with no subfolders at all (every file at the
  top level) shows exactly one row per file and no folder rows, same as
  before folders were handled specifically
- A folder row shows *aggregated* values across every file beneath it,
  however many levels deep: combined size, combined download
  percentage, and a `[X]`/`[ ]` checkbox for wanted — `[-]` when
  descendants disagree — or the priority level only when every
  descendant agrees ("Mixed" when they don't)
- Double-clicking a row does something specific to the column it lands
  on — the wanted column toggles it; the priority column cycles
  Low→Normal→High→Low (a "Mixed" folder resolves to Low first, same
  rule the wanted toggle already uses for its own ambiguous case).
  Right-click for the same two actions instead, plus jumping straight
  to a specific priority level the cycling can't reach directly — no
  buttons for either, both are mouse-driven only
- The same right-click context menu also has **Rename**, for the
  focused row, file or folder alike — prompts for a new name (just the
  last path segment; Transmission's own `torrent-rename-path` only ever
  renames that, not the whole path) pre-filled with the current one,
  then re-fetches the file list on success so every downstream detail
  (a renamed folder's own full path, every descendant file's own path
  if it was one, sort order shifting if the new name reorders it among
  its siblings) comes from the same source of truth as everything else
  here, rather than being patched in place for just this one case. A
  failed rename shows the error and changes nothing
- On a folder, "toggle wanted" turns every descendant off if they're
  all currently on, or turns them *all* on otherwise (including when
  they disagree) — so a folder in a "Mixed" state always resolves to
  "all wanted" first, never something ambiguous like flipping each file
  independently. Setting a priority on a folder (from the context menu)
  is simpler: it's just set on every descendant, no toggle involved.
  "Select all"/"Select none" (the two remaining buttons) do the wanted
  toggle for every file in the torrent at once — useful for quickly
  narrowing a large multi-folder torrent (a season of a show, say) down
  to just the files actually wanted, one folder at a time instead of
  one file at a time
- Every action applies immediately (`torrent-set`'s `files-wanted`/
  `files-unwanted`/`priority-low`/`priority-normal`/`priority-high`,
  addressed by file index — a folder simply lists every real file index
  beneath it) — there's no separate "Apply" step
- Marking an already fully-downloaded file "not wanted" doesn't delete
  it; Transmission just stops treating it as something to keep verifying
  and download further copies of
- This data (`files`/`fileStats`, part of `torrent-get`) is fetched only
  when this window is opened, the same as the details and tracker
  windows above, not as part of the periodic list refresh

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
  Reannounce, Remove, Delete (with files), Details, Files — right-clicking
  also selects that row first, even if it wasn't already focused
- Middle-click a row to jump straight to its files window — a one-click
  shortcut for the single most common reason to right-click and pick
  "Files" from the menu, without going through it
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
- A status bar shows the combined download/upload rate across every
  torrent in whichever torrent-list window currently has focus (not a
  grand total across every open server — see "Torrent list" above),
  refreshed on every UI tick (no extra RPC calls); shows placeholder
  dashes when no torrent-list window has focus at all

**Connection (F9, "Settings" menu)**
- Server name, refresh interval (seconds), host, port, RPC
  username/password, interface language
- "Server name" is an editable combo (see "Fixed bugs" below for the
  widget itself) — the logical name a set of host/port/user/password is
  saved under, e.g. "home" or "seedbox". Picking a different existing
  name from the dropdown, or typing one directly, loads that server's
  own saved details into the other fields automatically if it matches
  one already saved; typing (or switching to) a name that doesn't
  resets host/port/user/password to generic defaults instead of leaving
  whatever the previously-shown server's own details were — those
  aren't this (as yet unconfigured) server's details. "[+]" adds
  whatever's currently shown as a list entry on its own (with a
  confirmation popup naming it), without saving anything under it yet
- Host/port/user/password stay disabled — visibly dimmed, and skipped
  entirely by Tab — until the name currently shown is actually a
  registered entry in the combo, whether that's a name already saved
  from a previous session or one just added this session via "[+]".
  There's nothing meaningful to configure for a name that's neither, so
  editing those fields is blocked outright rather than left looking
  editable with nothing real behind it
- The button beneath — labeled **Save** or **OK** depending on
  whether what's shown right now already matches a saved server exactly
  — reflects whether there's anything new to persist. Save runs a real
  connection test (the same kind of RPC round trip "Server
  Configuration" already relies on for its own fetch) with whatever's
  currently in the fields; only on success does it actually save that
  server's details to disk and open (or update) its own torrent-list
  window, then relabel itself to OK — the dialog stays open either way,
  ready to configure another server next, or just close via the now-OK
  button. A failed test shows the error and changes nothing. Typing (or
  selecting) a different server, or editing host/port/user/password
  directly, puts the button back to reading Save — even right after a
  successful one, since now there's something new that hasn't been
  tested yet
- "[-]" is NOT symmetric with any of the above: removing a server, and
  confirming its own popup, takes effect immediately — closing that
  server's torrent-list window along with every Details/Files/Tracker
  window still open for one of its torrents, and saving the removal to
  disk right then (see "Fixed bugs" below) — rather than waiting for
  Save/OK, so pressing Cancel afterward does NOT undo it
- First step toward managing more than one server — right now exactly
  one is ever connected to at a time per Save (whichever was last
  successfully tested and saved); switching to a different saved one
  and doing the same is what "quickly switching between servers" means
  at this stage, not yet several running side by side
- Changing the language shows a popup noting that a restart is needed
  for the menu bar and status bar to relabel — everything else already
  has (see "Internationalization" below for why those two specifically
  lag behind)

**Server Configuration ("Server..." — "Settings" menu)**
- Global (session-wide) download/upload speed limits — read from and
  written straight to the Transmission daemon itself (`session-get` /
  `session-set`), not stored in this app's own settings file; these are
  the defaults any torrent without its own override (above) follows
- "Speed Limit" mode's own on/off switch and its own pair of (usually
  lower) limits — Transmission switches to these instead of the pair
  above as a whole while the mode is on
- A separate dialog from Connection above: this one's fields all live
  on the daemon rather than in this app's own settings file, fetched
  fresh with a live RPC call each time it's opened — never saved
  locally and reloaded from there, so it always reflects whatever the
  daemon's actual state is at the moment the dialog opens, including
  changes made some other way (another client, a script) since this
  app last checked
- A "Network" section with two on-demand daemon-side checks — "Test
  port" (is the daemon's own configured incoming peer port reachable
  from outside) and "Update blocklist" (re-download and reload the IP
  blocklist from whatever URL the daemon's already configured with —
  this app has no UI for setting that URL itself, only for triggering
  the update). Each shows its own result inline, next to its own
  button, the moment that one call finishes — not a popup, so it stays
  visible without needing to be dismissed, and doesn't interrupt
  anything else on the same dialog

**Session Statistics ("Session Statistics..." — "Settings" menu)**
- Current-session and all-time (cumulative) totals for the connected
  daemon: bytes downloaded/uploaded and time active, plus how many
  times the daemon itself has been started, ever — Transmission's own
  `session-stats` RPC, the same live-fetch-each-time approach as Server
  Configuration just above rather than anything stored in this app's
  own settings file
- Read-only: nothing here round-trips into a setting, so unlike Server
  Configuration there's no OK to confirm — just "Refresh" (re-fetches
  and updates every value in place) and "Close"
- Acts on whichever server's window currently has focus, same as
  Server Configuration and "Manage columns..."

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
- One window for everything about a grid's columns, replacing what
  used to be three separate entry points for the main list alone (a
  "Resize columns" submenu, an "Order columns" submenu, and a
  standalone "Columns..." checkbox dialog) — see "Fixed bugs" below for
  why
- A single menu entry, not one per window: it acts on whichever
  `TGridView`-based window currently has focus — the main torrent list,
  the tracker list, or any future window built the same way — rather
  than needing a separate "Manage columns" of its own in each one. Only
  enabled while a compatible window is focused; greyed out otherwise
  (a dialog like Settings or Filters, or a window with no grid in it at
  all, like the torrent details window)
- The dialog itself lives in `src/tvision-ext/` — it's not specific to
  the torrent list, or to this app at all: it operates on any
  `TGridView`, with this app just passing in its own translated text
  (see "Fixed bugs" below)
- Shown as a small grid of its own — one row per real column of
  whichever grid is focused (16 for the main list: the 7 shown by
  default, plus 9 more hidden by default — see below; 6 for the
  tracker list), with its label, current width, and a `[X]`/`[ ]`
  visibility marker — select a row, then:
  - **Resize**: the same keyboard-driven resize (Left/Right live,
    Enter confirms, Esc cancels) that dragging a column header
    separator does
  - **Move**: the same reorder ("<"/">" markers, Left/Right, Enter/Esc)
    that double-clicking a column's header does
  - **Toggle visible**: shows or hides it immediately — hiding doesn't
    lose its width or position, it just takes no screen space until
    shown again, reappearing exactly where it was. Double-clicking a row
    (or pressing Enter on it) does the same thing directly, without
    needing the button
  - **Reset**: puts width, order, AND visibility back to that grid's
    built-in defaults, all at once (the main list's 9 optional columns
    go back to hidden, not shown)
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
Transmission's own 0-based internal numbering). Bandwidth priority is
editable, not just shown: double-clicking that column cycles Low →
Normal → High → Low for the focused row (same mechanism as queue
position's own double-click cycling just below), and the same three
choices are also on a "Priority" submenu — both the main menu bar and
the list's own right-click context menu, right next to "Queue" —
applying to every currently selected torrent at once, or just the
focused one outside selection mode.

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
  from the Connection dialog via `TComboBox` — vendored into this
  project directly (see `src/tvision-ext/TComboBox.h` and "Fixed bugs"
  below) rather than pulled in from the tvision fork this project still
  points at; tvision itself has no built-in combo/dropdown control
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

Each configured server's own client keeps one persistent connection
(a single libcurl handle, reused for every request to that server —
see "Fixed bugs" below) rather than opening a fresh one per action;
every request still carries the current credentials and a 5s connect /
15s total timeout, so a request to an unreachable server fails
predictably instead of hanging indefinitely. Changing settings from the
TUI applies them immediately and triggers a refresh right away, so a
mistake shows up at once (an empty list); the CLI's `list` command
distinguishes a genuinely empty torrent list from a failed connection
via the RPC client's last-error state, and exits non-zero on failure.

The main list's own periodic refresh runs without blocking the rest of
the app (libcurl's own "multi" interface, one shared handle driven once
per event-loop tick — see "Fixed bugs" below) — a slow or unreachable
server no longer freezes every other open window, or the app as a
whole, for however long that one request takes. Every other action
(start, stop, remove, add a torrent, ...) is still an ordinary
synchronous call, each one short and a direct response to something
just clicked, now bounded by the same timeout either way. A server
whose most recent refresh attempt failed shows "(offline)" in its own
window's title bar until the next one succeeds — a persistent, glanceable
marker rather than a popup that would otherwise reappear every single
refresh interval for as long as that server stays down.

The main list's periodic refresh (`listTorrents()`/its own non-blocking
equivalent) only requests a lightweight set of fields — the extra detail
shown in a torrent's details window or tracker list (location, magnet link,
piece info,
all-time totals, per-tracker stats, ...) is fetched with its own
separate request, made only when that window is opened (or its
"Refresh" button pressed, for trackers), so the fields most torrents'
rows don't need aren't carried on every refresh tick for every torrent.

## Configuration file

Settings (every saved server's own host/port/user/password — see
"Connection" above — plus which one is active, each server's own
column widths/order/visibility, and which one had focus when the app
last closed — see "Torrent list" above — refresh interval, language,
the torrent list's last sort column/direction (still one shared
setting, not per server), the tracker list's own column widths/order/
visibility (shared the same
way), and the active filter) are stored in:

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
last change. The tracker list's own columns are saved and restored the
same way, in their own separate `settings.json` fields — there's one
shared tracker column layout, not one per torrent, so opening a tracker
window for a different torrent later still starts from whatever was
last saved. If several tracker windows happen to be open at once when
the app exits, the shutdown backstop just picks whichever one it finds
first, since they're meant to share one layout rather than needing to
be reconciled against each other.

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
   This is the real upstream tvision — no fork needed. It used to point
   at [zanac/tvision](https://github.com/zanac/tvision) instead, a fork
   that originally added `TComboBox` (tvision has no drop-down combo box
   built in; see [issue #173](https://github.com/magiblot/tvision/issues/173),
   open since 2025 with no resolution). That class has been fully
   vendored into this project since (`src/tvision-ext/TComboBox.h/.cpp`),
   including the one thing that still silently depended on the fork
   after that — see "Fixed bugs" below — so there's nothing left here
   that needs anything beyond stock upstream tvision.
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
    TorrentFilesWindow.h/.cpp   Per-file selection/priority window (built on TGridView)
    TrackerPeerWindow.h/.cpp    Per-torrent tracker/peer window, two tabs (opened from the details window)
    TrackerDetailWindow.h/.cpp  Full status for a single tracker (double-click a row)
    AddTorrentDialog.h/.cpp     "Add torrent" dialog
    ConnectionDialog.h/.cpp     "Connection" dialog
    ServerSettingsDialog.h/.cpp "Server Configuration" dialog
    FilterDialog.h/.cpp         "Filters" dialog
    LanguageComboBox.h/.cpp      Compact custom combo box for the language picker
    WindowListDialog.h/.cpp     "Window list" dialog
    AboutDialog.h/.cpp          "About" dialog
    BandwidthStatusLine.h/.cpp  Status bar with a runtime-updatable item
packaging/
  appimage/
    build-appimage.sh          Builds build/TvTransmission-x86_64.AppImage
    tv-transmission.desktop     Desktop entry (Terminal=true)
    tv-transmission.png         Placeholder icon
  tvision-ext/
    TGridView.h/.cpp           Generic dynamic-column list view — see
                                TGridView-README.md; self-contained, no
                                dependency on the rest of this project, meant
                                to be copied into other Turbo Vision projects
                                wholesale (or eventually proposed as a PR to
                                tvision itself — see "Fixed bugs" below).
                                Used by TorrentListWindow (ui/) for the main
                                list's rendering.
    TGridWindow.h/.cpp          Optional TWindow wrapper (fullscreen-locked or
                                ordinary MDI child) hosting one TGridView —
                                TorrentListWindow (ui/) derives from this
    TGridColumnManagerDialog.h/.cpp  Reusable "resize/move/show/hide columns"
                                window for any TGridView — text customizable via
                                TGridColumnManagerLabels (this module has no
                                dependency on any app's own translation system);
                                used by App.cpp (ui/) with this app's own
                                translated strings passed in
    TGridView-README.md         Full design writeup for TGridView specifically
    TComboBox.h/.cpp             Vendored TComboBox + TComboItem/TComboViewer/
                                TComboWindow, plus an editable mode (typed text,
                                "[+]"/"[-]" list buttons) added on top — see the
                                file's own header comment. Used by LanguageComboBox
                                (ui/, non-editable) and ConnectionDialog (ui/,
                                editable — the server-name combo).
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

- **Change download location** for an already-added torrent
  (`torrent-set-location`)
- **Per-torrent seed ratio limit**, distinct from a speed limit
  (`seedRatioLimit`/`seedRatioMode` in `torrent-set`)

## Fixed bugs

Kept here for context, in case similar patterns come up again.

**`TGridView`'s own default row color, unified — one shared fallback
instead of the same callback copy-pasted into four separate files.**
Asked for directly: the main torrent list's own rows (colored by
status — yellow for checking/queued, cyan for downloading, ...) looked
visually inconsistent with every other `TGridView`-based window in this
app, which all showed a fixed white-on-blue instead.

Not actually a difference in INTENT — every one of those four windows
(`TrackerPeerWindow`, `TorrentFilesWindow`, `TFolderBrowserDialog`,
`TGridColumnManagerDialog`'s own meta-grid) already set the exact same
`setRowColorCallback` explicitly: white-on-blue normally
(`TColorAttr(0x1F)`), black-on-white when focused (`TColorAttr(0xF0)`),
because `TGridView`'s own fallback (when no callback is set at all)
resolved colors through the owning dialog's own palette chain
(`getColor(1)`/`getColor(2)`), which doesn't contrast enough inside a
`TDialog` to actually see which row is focused. Four identical copies
of the same four-line callback, purely because the generic widget's own
built-in fallback wasn't already doing this.

Fixed by moving that exact pair of colors into `TGridView` itself, as
what happens when NO callback is set, rather than something every
caller needs to remember to set — `setRowColorCallback()` still fully
overrides it when a caller genuinely needs something else (the main
list's own per-status coloring being the one case that actually does),
now purely additive rather than a workaround every other caller had to
duplicate. Removed the four now-redundant explicit callbacks entirely.

Verified two things separately, not just that removing four blocks of
duplicate code still compiled: that the main list's own per-status
colors are UNCHANGED (this couldn't have been reachable regardless —
`TGridView`'s new fallback only ever applies when `rowColor_` isn't
set, and the main list always sets its own, so the changed code path is
provably unreachable there, not just observed to still work); and that
a window that used to set the callback explicitly (`TrackerPeerWindow`)
shows the exact same color now that it relies on the new default
instead — read directly from a live running instance's own cell
attributes (`fg=brightwhite bg=blue`, i.e. exactly `TColorAttr(0x1F)`),
not just "looks about right."

**The destination folder chosen in "Add torrent" was never actually
sent anywhere — the whole feature had no effect on where a torrent
actually got saved.** Found while checking something else entirely
(how a wrong/nonexistent destination gets handled), not from a report
against the feature itself — worth documenting since it's the kind of
gap that's easy to miss: everything ELSE about the feature worked
(the label showed the chosen folder, "Verify" showed its free space,
switching folders refreshed both correctly), so there was nothing
visibly broken to notice.

`TransmissionClient::addTorrent()` only ever sent `{"filename":
urlOrPath}` to `torrent-add` — no `download-dir` argument at all, so
Transmission always used its own default regardless of what "Add
torrent" showed as the destination. `App::showAddTorrentDialog()`'s own
`cmOK` handling had `destination` sitting right there, already captured
for the Browse/Change reopen cycle, and simply never passed it to
`addTorrent()`. Fixed by giving `addTorrent()` an optional
`downloadDir` parameter (empty by default, so the CLI's own call site
— which has no destination concept — needed no change at all) that
becomes `torrent-add`'s own `download-dir` argument when non-empty, and
passing `destination` through from the one call site that has one.

This also surfaced a real, worth-naming limitation rather than a bug to
fix: `TFolderBrowserDialog`'s own folder list only ever looks at the
LOCAL filesystem (this app's own machine), while `download-dir` is a
path on the DAEMON's own filesystem — the same machine only when
managing a local `transmission-daemon`, not necessarily so for a remote
one. RPC has no "list a directory on the daemon" method to browse the
real one instead, so this is accepted as a known gap rather than
something to work around — picking a folder that exists locally but not
on a remote daemon is possible, and would fail the same way any other
invalid `download-dir` does (see the verification below).

Verified two things together, not just that the fix compiled: first,
that `download-dir` actually reaches the RPC request with the exact
value shown in the dialog — confirmed by having a mock log every
`torrent-add` call's own arguments and checking both a default-folder
add and a changed-folder add each logged the right one. Second, that a
destination the DAEMON itself rejects (simulated: locally readable, so
the folder browser accepts it, but the mock's own `torrent-add` handler
treats that specific path as invalid) surfaces Transmission's own error
text in a normal error messageBox — "Failed to add the torrent: No such
directory: ..." — rather than failing silently or crashing, reusing the
exact same error-reporting path duplicate/invalid-torrent failures
already went through.

**A real use-after-free, found while building the folder browser below —
`close()` used on a MODAL dialog instead of the correct `endModal()`.**
Two dialogs (`SessionStatsDialog`'s own "Close" button, and this same
new `TFolderBrowserDialog`'s own first version of its "Cancel" button)
both used a custom command handled with `close()` — the same idiom this
app's own NON-modal windows (`TorrentDetailsWindow`, `TrackerPeerWindow`,
and others, all opened via `insertWindow()`) already use correctly for
their own Close buttons. The difference that matters: `TWindow::close()`
(`twindow.cpp`) calls `destroy(this)` directly — deleting the object
immediately. For a non-modal window that's fine; nothing else is
waiting on it. For a MODAL dialog (opened via `execView()`), it's a
genuine use-after-free: `TGroup::execute()`'s own event loop (`do {
getEvent(e); handleEvent(e); } while (!valid(endState))`) is still
running further up the very same call stack when `handleEvent()`
returns, and it goes on to call `valid()` on `this` — now a dangling
pointer, since `close()` already freed it moments earlier from inside
that same `handleEvent()` call.

Confirms `close()` genuinely deletes the object (not just calling this
class's own hoped-for behavior for it), by reading `TWindow::close()`
directly rather than assuming: `if (valid(cmClose)) { frame = 0;
destroy(this); }`. `TDialog`'s own base `handleEvent()` already turns
the STANDARD commands (`cmOK`/`cmCancel`/`cmYes`/`cmNo`) into
`endModal()` on its own — correctly, safely — which is why every OTHER
dialog in this app (`ServerSettingsDialog`, `ConnectionDialog`, ...)
never needed a custom Close/Cancel handler of its own at all. Fixed
both by removing the custom command and its handler entirely, using
plain `cmCancel` on the button instead — letting the exact same
built-in mechanism every other dialog already relies on handle it,
rather than reinventing (and this time, breaking) it.

Not caught by reading the code alone: found from a live run where
Cancel-ing the new folder browser silently failed to return control to
"Add torrent" afterward — the dialog visually disappeared (matching
what `close()`'s own `destroy(this)` does), but the app never resumed
as expected, which is the outward symptom a `valid()` call on freed
memory tends to produce rather than an immediate, obvious crash.
Verified fixed the same way: the identical Cancel action, followed by
confirming "Add torrent" reopens correctly showing its own previous
destination and free-space values unchanged, i.e. that the whole
close→reopen round trip actually completes end to end now.

**New: destination folder and free disk space in "Add torrent," and
a new reusable folder-picker dialog to go with it.** Genuinely new
functionality, not a bug fix — the folder-browser dialog itself, and
the design reasoning behind it (why not tvision's own `TChDirDialog`),
is documented on its own further down; this entry is specifically
about wiring it into "Add torrent."

"Add torrent" gained a destination label (the server's own default
download directory at first, changeable via a new "Change..." button)
and a free-space label next to it (fetched automatically the moment
the dialog opens, plus its own "Verify" button to re-check later) —
`TransmissionClient::getDefaultDownloadDir()`/`getFreeSpace()`, both new,
both the same synchronous `call()`/15s-timeout pattern every other
action in this class already uses. "Change..." follows the exact same
"close this dialog first, then open the next one, then reopen this one
again" pattern already established for "Browse..." (see the entry
further down on why nesting a second modal dialog directly inside this
one isn't safe) — opening the new folder-browser dialog below rather
than nesting it, then reopening "Add torrent" with whatever was chosen,
alongside whatever URL/file value was already typed, so neither field
loses what was there before the other one changed.

Verified end to end on a live running instance, not just that each
piece compiled: opened "Add torrent" and confirmed the destination and
free space both populated correctly without any click needed first;
clicked "Change...", navigated into a subfolder, confirmed with
"Select" — back in "Add torrent," both the destination AND the free
space had updated to that subfolder's own specific value (a mock
returning a genuinely different number per exact path, not the same
value regardless — otherwise a stale number would have looked
identical to a correctly refreshed one).

**New: `TFolderBrowserDialog`, a from-scratch folder-picker added to
`tvision-ext/`.** Built specifically to back "Change..." above, after
concluding tvision's own `TChDirDialog` was a dead end for this exact
purpose — Windows-path assumptions baked into its own public API, a
still-open memory-safety report against it, and even the one person
who'd tried adapting it for "choose a folder" specifically had given up
and written a new one instead (all found by actually reading that
project's own GitHub history — issue #137, PR #141 — rather than
assuming `TChDirDialog` would just work). Reaches the same conclusion,
independently, for the same reason.

Deliberately minimal, matching what was actually asked for rather than
building in extra scope: no tree view, no "New Folder" button — a
single-level navigator (list the current directory, double-click or
Enter to descend, ".." to go back up), the same model tvision's own
`TFileDialog` already uses for picking a FILE, applied here to picking
a DIRECTORY instead. Built on this project's own `TGridView` rather
than tvision's `TOutline`, for the same reason `TorrentFilesWindow`'s
own file tree already is — one consistent look across every list in
an app that embeds this, not a second, differently-styled list widget
appearing only here. The path field at the top is directly editable —
typing a path and pressing Enter navigates there, intercepted before
`TDialog`'s own base `handleEvent()` gets a chance to treat Enter as
"press whatever button has `bfDefault`" instead (a plain `TInputLine`
doesn't consume Enter on its own, so without this, typing a path and
pressing Enter would have instead tried to confirm the dialog with
whatever was ALREADY in `currentPath_`, ignoring what was just typed).
No dependency on this app's own translation system, matching
`TGridColumnManagerDialog`'s own established pattern for this same
codebase: every label passed in via a `TFolderBrowserLabels` struct
with plain-English defaults, built from this app's own `tr()`-based
strings right before opening it.

**New: a "Session Statistics" dialog.** Genuinely new functionality, not
a bug fix — an idea floated much earlier in this project's own history
and only now actually built.

`TransmissionClient::getSessionStats()`: a new `SessionStats` struct
(current-session and cumulative/all-time byte counts, active-time, and
daemon start count) fetched via the `session-stats` RPC method — the
same `ok`-reports-success-vs-genuine-zero convention
`getSessionLimits()` already uses, since a torrent-free daemon
genuinely reports all zeros here and that's not itself a failure to
distinguish from a broken connection.

The dialog itself (`SessionStatsDialog.h/.cpp`, a new file pair) is
simpler than Server Configuration in one specific way: every field here
is read-only — nothing round-trips into a setting, so there's no
`...Result()` function or `Fields` struct to read back once closed,
just "Refresh" (re-fetches and updates every label in place) and
"Close." Reused Server Configuration's own `TResultLabel` idea (a
`TStaticText` with a `setText()` the base class doesn't have) rather
than sharing the literal class — both are a few lines, file-local, and
this project doesn't otherwise have a home for a view helper used by
exactly two unrelated dialogs.

Verified against a mock returning specific, checkable values (200MB/
100MB for the current session, 10GB/5GB all-time, matching seconds-
active figures, and a session count of 42) on a live running instance:
every single field matched exactly, not just that the dialog opened
without crashing.

**New: per-torrent bandwidth priority is now editable, not just
shown.** Genuinely new functionality, not a bug fix — worth documenting
since it follows an established pattern (queue position's own
double-click cycling) closely enough that it's worth noting exactly
where it diverges from it and why.

`TransmissionClient::setPriority(torrentId, priority)`: the same
`torrent-set` RPC method `setFilesPriority()` already used, but setting
`bandwidthPriority` directly (a single value for the whole torrent)
rather than one of the per-FILE `priority-low`/`priority-normal`/
`priority-high` index arrays that method uses — a torrent's own
priority and a file's own priority are two different fields entirely,
not the same concept at two different scopes.

The double-click cycle diverges from queue position's own
`cycleQueueActionForRow()` in one deliberate way: queue position's four
actions (top/up/down/bottom) are RELATIVE moves with no "current state"
of their own to read, so that method cycles through a single shared
counter (`queueActionCycle_`) regardless of which row was actually
clicked. Priority is different — `Torrent::bandwidthPriority` already
says exactly where in the Low/Normal/High cycle a given row currently
is — so `cyclePriorityForRow()` reads that row's own current value
directly and computes the next step from it, with no shared counter to
keep in sync with anything.

The explicit "Priority" submenu (Low/Normal/High) went in both places
"Queue" already was — the main menu bar's own Torrent menu, and the
list's own right-click context menu — reusing the exact same nested-
TSubMenu construction (and the same `(TMenuItem&)` cast) queueMenu
already needed in both those places, rather than inventing a different
pattern for what's structurally the same kind of addition.
`setPriorityForSelected()` follows `queueMoveTopForSelected()`'s own
shape exactly: every currently selected torrent via `targetTorrents()`,
or just the focused one outside selection mode.

Verified on a live running instance against a mock that actually
remembers `bandwidthPriority` between calls (a mock that always
reports the same fixed value regardless of what was just set wouldn't
have caught anything — the cycle would look identical whether it
worked or not): three double-clicks on the Priority column moved
Normal → High → Low → Normal, matching the cycle exactly, and the
right-click context menu showed "Priority ►" appearing right next to
"Queue ►", as intended.

**New: "Test port" and "Update blocklist" in the Server Configuration
dialog.** Genuinely new functionality, not a bug fix — worth documenting
the design since it's the first place in this app that needs a dialog
to act on a live `TransmissionClient` WHILE still open, rather than
only reading its fields back once it closes.

`TransmissionClient::testPort()`/`updateBlocklist()`: the same
synchronous `call()` every other action in this class already uses
(`port-test`/`blocklist-update`, both session-level RPC methods with no
arguments), bounded by the same 15s timeout. Neither takes a URL or a
port to check — Transmission always tests/updates against whatever
it's already configured with; this app has no UI for setting the
blocklist's own URL, only for triggering the daemon's already-configured
update.

Every other field on this dialog only round-trips through
`ServerSettingsDialogFields`/`serverSettingsDialogResult()` once the
dialog closes with OK — these two needed to call straight into the
client and show a result WHILE the dialog stays open, so
`ServerSettingsDialogImpl` (a small `TDialog` subclass, previously this
was built directly from a plain `TDialog` with no subclass at all) now
holds a `TransmissionClient&` and handles its own two buttons' commands
directly. A popup for the result was deliberately not used — a
messageBox for something the user can just look at again a moment
later, right there in the same dialog, would be one more thing to
dismiss for no benefit — instead each button has its own small label
right next to it (`TResultLabel`, a `TStaticText` subclass with a
`setText()` the base class doesn't otherwise have, replacing the same
way this project already replaces a `TView`'s own title elsewhere).

One thing that needed getting right rather than skipped: both calls
block the whole app for however long the round trip takes (same as
every other action), so "Testing..."/"Updating..." was set on the label
right before the call starts — but `drawView()` alone only updates
tvision's own in-memory screen buffer, which doesn't reach the real
terminal until control returns to the event loop, by which point the
blocking call would already be finished and there'd be nothing left to
show that text for. `TScreen::flushScreen()` forces an immediate real
repaint right after setting it, so the intermediate text is actually
visible for however long the call takes, not silently skipped over.

Verified on a live running instance against a mock returning
`port-is-open: true` and `blocklist-size: 123456`: clicking "Test port"
showed "Port: open", clicking "Update blocklist" showed "Blocklist:
123456 rules" — the exact values the mock sent, not just that the
buttons did something.

**Rapid switching between the Trackers and Peers tabs could eventually
leave the list stuck showing nothing — two unrelated code paths were
silently sharing one curl handle they should never have shared.**
Reported directly ("switching quickly back and forth enough times, it
eventually gets stuck").

The cause: `TransmissionClient` kept exactly one persistent easy handle
(`curl_`), reused by every synchronous call (`call()` — including
`getTrackerStats()`/`getPeers()`, which is what each tab switch
triggers) AND by the periodic async refresh (`startRefresh()`/
`finishRefresh()`, which adds that same handle to a `CURLM*` via
`curl_multi_add_handle()` and doesn't remove it again until the
transfer completes). libcurl's own documentation is explicit that an
easy handle currently attached to a multi handle must not be reused for
a separate, direct `curl_easy_perform()` until it's been removed —
which is exactly what happened whenever a tab switch's own synchronous
call landed while the SAME window's periodic refresh had that handle
mid-transfer: `call()` calls `curl_easy_reset()` and reuses it anyway,
resetting state curl's own multi-handle bookkeeping was still relying
on. This is undefined behavior, not a clean, loud failure — matching
the reported symptom exactly: a stuck, empty list rather than a crash.

Fixed by giving `TransmissionClient` a SECOND, entirely separate easy
handle (`refreshCurl_`), used only by `startRefresh()`/`finishRefresh()`
— `call()` keeps using `curl_` exclusively, and the two code paths can
no longer collide no matter how the timing lines up. The one cost:
a call from `getTrackerStats()`/`getPeers()`/etc. no longer reuses the
exact same underlying TCP connection the periodic refresh's own handle
already has open to the same host, opening a second connection
instead — trivial next to correctness here.

Verified as a real, reproducible bug, not a theoretical one, both
before and after the fix: a threaded mock server (deliberately slow on
the main list's own refresh, fast on tracker/peer requests, so the two
could genuinely overlap rather than just queue behind each other) and a
1-second refresh interval, then 20 rapid alternating clicks between the
two tabs on a live running instance. Reverting the fix back to one
shared handle reproduced real corruption within seconds — a details
window opening with every field blank, not the crash originally
expected, underscoring how this kind of bug tends to show up as
"garbled" rather than "loud." The same 20-click sequence against the
actual fix: correct data on every switch, main list still refreshing
normally throughout, process alive and responsive at the end.

**A real crash on every normal exit — `App`'s own destructor reading
through a null `deskTop` pointer.** Reported directly, then reproduced
and confirmed with a debug build under AddressSanitizer rather than
guessed at from the code alone: `App::~App()` used to loop over
`allListWindows()` first, canceling any in-flight async refresh before
cleaning up the shared `multiHandle_` — reasoning that every easy
handle needs detaching before `curl_multi_cleanup()` runs on it (still
true), and that doing it explicitly here was safer than depending on
exactly when each window's own destructor happened to run relative to
this one.

That reasoning had a wrong assumption baked in: it assumed this
destructor's own body runs BEFORE the windows themselves are torn
down. It doesn't. By the time control reaches `~App()`, `TApplication::
run()` has already called `shutDown()` as part of its own normal exit
path — and tvision's own `TProgram::shutDown()` (`tprogram.cpp`) sets
`deskTop = 0` before `TGroup::shutDown()` actually destroys every child
view, cascading into each `TorrentListWindow`'s own `TransmissionClient`
destructor, which ALREADY detaches safely from `multiHandle_` if a
refresh happened to be in flight — the exact same safety net this loop
was trying to provide again, just redundantly, and via a `deskTop`
pointer that was already null by the time it ran. `allListWindows()`
dereferences `TProgram::deskTop` without a null check, which is exactly
where AddressSanitizer caught the SEGV: `App::allListWindows() ←
App::~App() ← main()`, on every single normal exit, not some rare edge
case.

Fixed by simply removing the loop — `~App()` now only cleans up
`multiHandle_` itself, since every client has already safely detached
from it on its own by the time this destructor's body runs. Also
removed `TransmissionClient::cancelRefresh()`/`TorrentListWindow::
cancelAsyncRefresh()`, the two methods that loop existed to call —
dead code once nothing calls them anymore, not kept around "just in
case."

Verified thoroughly, not just "no crash on a quick exit": a debug
build under AddressSanitizer, first confirming the exact crash from a
plain Alt-X quit, then confirming it was gone after the fix — and, to
make sure the fix didn't just get lucky by exiting too fast to matter,
a second run with a deliberately slow mock server (3s per response)
and a 2-second refresh interval, sending Alt-X while a periodic async
refresh was confirmed still in flight. Clean exit, code 0, no
AddressSanitizer error, in both cases.

**A stray blank row above the Trackers/Peers grid.** Marked directly on
a screenshot: one row of empty space between the radio cluster and the
column headers below it, left over from when that space was originally
sized for two `TButton`s two rows tall each — the radio cluster
replacing them is only one row tall, but the layout code reserving
space above the grid was never adjusted down to match. Fixed by
reserving one row instead of two.

**Column headers and the radio cluster's own background, both matching
the rows' blue — two separate gaps, closed with one shared trick.**
Asked for directly, marked on a screenshot: the cluster's own teal
background stopped right after "Peers" instead of continuing to the
window's own right edge, and the column headers below it used the
default palette color instead of matching the rows.

The radio cluster: `TCluster::drawMultiBox()` (in tvision's own
`tcluster.cpp`) already fills its ENTIRE bounds with one color before
drawing anything on top (`b.moveChar(0, ' ', cNorm, size.x)`) — nothing
needed patching here at all, unlike the shading `TButton::drawState()`
turned out to do piece by piece (see the entry further down this file
on why button-based tabs were abandoned for this same cluster). The
gap was simply that the cluster's own bounds were only wide enough for
the two labels, not the full row — widened to the grid's own full width
below it, and its background now fills the whole row on its own, no
override needed.

The column headers needed a genuinely new hook, since `TGridHeaderView`
(part of the generic `TGridView` widget) always resolved its own color
through the owning dialog's palette chain, with no way for an embedding
window to override it directly — unlike each ROW's own color, which
already had exactly this kind of hook (`RowColorFn`/
`setRowColorCallback()`). Added the equivalent for the header:
`HeaderColorFn`/`setHeaderColorCallback()`, optional (falls back to the
existing palette-chain behavior when unset, so every other
`TGridView`-based window in this app — the main list, the files window
— keeps looking exactly as it already did, unaffected).
`TrackerPeerWindow` sets it to the exact same hardcoded `TColorAttr`
its own rows already use for the unfocused case, so the header visibly
matches rather than coincidentally happening to look similar.

Verified both directly on a live running instance: every cell's own
background color read individually from right after "Peers" to near
the window's own right edge — uniform the whole way, confirming the
cluster's own fill genuinely reaches the edge rather than just looking
close in a quick glance. The header: read the same way against a data
row beneath it — identical background on both.

**The Trackers/Peers switch, rebuilt on a native `TRadioButtons`
cluster instead of two ordinary buttons pretending to be tabs.** The
original version of this (two plain `TButton`s, whichever one active
simply disabled) worked, but every attempt at making it actually LOOK
like two attached tabs — a distinct color for the active one, no gap
between them, no residual shading anywhere on either button — kept
running into another edge `TButton::drawState()` shades that hadn't
been patched yet: first the left/right edges of each button, then a
whole extra shadow row underneath, each only found from a fresh
screenshot after the previous fix seemed to check out. None of that
was actually necessary: `TRadioButtons` is tvision's own control for
exactly this "choose one of a small fixed set" case, complete with its
own selection marker (a filled vs. empty circle, `(•)`/`( )`), keyboard
navigation, and focus handling, none of which needs reimplementing or
patching around piece by piece. Decided to stop refining the
button-based version and replace it outright rather than continue
finding and fixing one more edge case each time.

`TTrackerPeerRadio` (a small `TRadioButtons` subclass, local to this
window) overrides `press()`/`movedTo()` — both change which item is
selected in the base class, `movedTo()` on arrow-key navigation without
a separate confirm step, `press()` on an actual click — to also invoke
an `onChanged` callback, the same callback-based pattern already used
throughout this app for other custom controls. `TCluster`'s own layout
switches between a vertical list and side-by-side columns based purely
on the bounds it's given: a height of exactly one row is what makes
"Trackers"/"Peers" lay out horizontally like tabs, rather than stacked
on top of each other the way a taller cluster would default to.

Verified on a live running instance, not just compiled: the marker
correctly starts on "Trackers", clicking "Peers" moves it there and
switches the grid to show peer data (address, client, progress, speeds)
matching the mock's own — confirming both the visual selection and the
underlying tab-switching logic survived the rewrite intact, not just
that a `TRadioButtons` cluster renders without crashing.

**`TrackerListWindow` renamed to `TrackerPeerWindow`.** Pointed out
directly, right after the Peers tab landed (see the entry just below
this one): the file and class name still said "tracker list" even
though the window itself now shows trackers *or* peers, switched via
its own two tabs — accurate right up until that entry, misleading
afterward. Renamed the files (`TrackerListWindow.h/.cpp` →
`TrackerPeerWindow.h/.cpp`) and the class/factory function
(`TrackerListWindow`/`createTrackerListWindow` → `TrackerPeerWindow`/
`createTrackerPeerWindow`) together, updated every reference across the
app (`App.cpp`, `TorrentDetailsWindow.h/.cpp`, `TorrentFilesWindow.h/
.cpp`'s own comments, `TrackerDetailWindow.h`'s own comment,
`TransmissionClient.h`'s own comment, `AppSettings.h`'s own comments,
`CMakeLists.txt`) — confirmed nothing missed by grepping the whole
`src/` tree for the old name afterward and finding nothing left,
rather than trusting the rename was complete just because it compiled.

**New: a Peers tab alongside the tracker window's own Trackers one,
switched via two buttons acting as tabs.** Genuinely new functionality,
not a bug fix — worth documenting the design since it's a pattern this
project hadn't used before (Turbo Vision has no tab control of its own).

`TransmissionClient` gained `getPeers()` alongside the existing
`getTrackerStats()` — same shape, a new `Peer` struct (`address`, `port`,
`clientName`, `progress`, `rateToClient`/`rateToPeer`, and
Transmission's own compact `flagStr` rather than decoding each flag
letter individually, matching how most other clients show it too),
fetched from `torrent-get`'s own `"peers"` field the same on-demand way
`trackerStats` already was — neither is part of the regular periodic
refresh; each is only requested when its own tab is the one actually
showing.

The tabs themselves: two `TButton`s at the top of the same window,
whichever one is the CURRENTLY active tab simply disabled — no separate
"selected" visual of its own was needed, since a button you can't click
because you're already looking at it reads as its own state indicator,
and this avoids inventing a highlight color that would have to somehow
not clash with the theme already established for “disabled” elsewhere.
One `TGridView` is reconfigured on every switch (`clearColumns()`, then
whichever tab's own columns) rather than keeping two separate grids,
which keeps “Manage columns…” working exactly as it already did for
every other grid in this app: it always acts on the ONE grid that
currently has focus, without needing to know or care that this window
happens to show two different things at different times. The trickiest
part of that: each tab needed its OWN remembered column layout, so
switching to Peers and back to Trackers within the same running window
doesn't lose whatever was resized on Trackers in the meantime, even
before "Manage columns..." ever explicitly saved anything — handled by
capturing the OUTGOING tab's current grid state into its own member
vectors right before rebuilding the grid out from under it on every
`switchToTab()` call, not just once at construction.

Persisting each tab's own layout across restarts needed threading two
new `AppSettings` fields (`peerColumnWidths`/`Order`/`Visible`,
alongside the existing tracker ones) through the same four-file chain
those already went through — `App` → `TorrentListWindow` →
`TorrentDetailsWindow` → `TrackerPeerWindow` (renamed from
`TrackerListWindow` right after this same feature landed — see the
entry right below this one for why) — doubling every constructor
parameter, stored member, and forwarded call along the way.
`App::showColumnManagerDialog()` and its own `shutDown()` backstop both
needed to know WHICH pair to save into, not just that a tracker window
had focus — `TrackerPeerWindow::isPeersTabActive()` is what tells them,
reusing the same `columnWidths()`/`columnOrder()`/`columnVisibility()`
getters either way, since those already just reflect whichever tab is
currently live in `grid_`. Fixed a small pre-existing gap while in that
same code either way: the `shutDown()` backstop was only ever capturing
`trackerColumnWidths`/`Order`, never `trackerColumnVisible` — a
direct show/hide via "Manage columns..." was already caught by its own
save-on-close, so this only mattered for column visibility changed some
other way, but it's now captured consistently with the width/order
fields right next to it.

Verified on a real running instance against a mock providing both
`trackerStats` and `peers` data together: opening the Trackers tab
showed the expected tracker row correctly; clicking over to Peers
showed both mock peers with their address:port, client name, progress
percentage, and down/up speeds all matching what the mock actually
sent — not just that the window opened without crashing, but that the
right data landed in the right columns on both tabs.

**The last silent dependency on the `zanac/tvision` fork, removed —
this project now builds against stock upstream tvision.** Found the
hard way: building against `magiblot/tvision` (the real upstream, per
the setup guide someone was actually following) failed with
`cmComboBoxSelectionChanged` undeclared in three different files.
`TComboBox` itself had already been fully vendored into this project's
own `src/tvision-ext/` (see the entry above this one for that work) —
but the vendoring missed one thing: `cmComboBoxSelectionChanged`'s own
comment explicitly said it "isn't declared here" because the fork it
was copied from had already added it directly to its own copy of
`views.h`, unconditionally. That was true right up until someone
actually tried building against a `tvision` checkout that ISN'T that
fork — at which point relying on a header this project doesn't control
to already declare a constant this project's own code depends on
turned from a true statement into a silent, unverified assumption.

Checked properly rather than guessed at before fixing it: neither
`TComboBox` nor any of its own command constants exist anywhere in
`magiblot/tvision` at all (a combo box was never part of classic Turbo
Vision; confirmed by grepping a fresh clone of upstream directly,
matching this project's own README note on
[issue #173](https://github.com/magiblot/tvision/issues/173) still
being open with no resolution) — so this was never actually about
`TComboBox`'s own class definition needing anything fork-specific, only
this one constant's declaration living in the wrong place. Fixed by
declaring all three of this widget's own broadcast commands
(`cmComboBoxSelectionChanged` alongside the two — `cmComboBoxItemAdded`/
`cmComboBoxItemRemoved` — that were already declared locally, added in
an earlier turn without the same oversight) directly in this project's
own `TComboBox.h`, at 59/60/61 — the same values as before, chosen as
the next three free slots above upstream's own highest built-in command
(`cmTimerExpired = 58`, confirmed directly in upstream's own `views.h`,
not assumed), so nothing about the actual protocol between this file
and its own three consumers (`ConnectionDialog.cpp`, and `TComboBox.cpp`
itself) needed to change at all — only where the numbers came from.

Verified as thoroughly as the original report deserved: a full build
from a clean checkout of `magiblot/tvision` itself (not the fork, not
this project's own `external/tvision` as it happened to be checked out)
— zero errors, and the resulting binary's own TUI and CLI both
confirmed working, the same way every change in this file gets
verified. The setup instructions above, and the submodule this project
itself is built against, both switched to upstream accordingly — this
was the one thing standing between "vendored, but only really if you
happen to build against the right fork" and actually not depending on
`zanac/tvision` at all anymore.

**Four related networking changes at once: a persistent connection per
server, a request timeout, non-blocking periodic refresh, and network
errors surfaced in the UI.** Discussed as a deliberate architectural
question rather than found as a bug — worth documenting the reasoning
in full, since the four build on each other and the riskiest one needed
real verification, not just review.

**Persistent connection.** `TransmissionClient` used to
`curl_easy_init()`/`curl_easy_cleanup()` a fresh handle on every single
call — no connection reuse at all, a new TCP handshake every time even
for a refresh hitting the same host every few seconds. Confirmed nothing
in this codebase ever copies a `TransmissionClient` (always by
reference, or held in a `std::unique_ptr` — see `App`'s own `clients_`
map) before adding a raw owned handle to it; copy and move are now
explicitly deleted too; a class member is only clearly safe to add once
nothing could end up owning two of it. One handle (`curl_`) now lives
for the object's whole lifetime, with `curl_easy_reset()` at the start
of every `call()` clearing whatever the previous one left set — reusing
the connection is the only thing that persists on purpose; every option
still gets set fresh, same as a brand new handle would need.

**Timeout.** `CURLOPT_CONNECTTIMEOUT`/`CURLOPT_TIMEOUT` (5s/15s) — there
was none before, so an unreachable server could block for however long
the OS's own TCP-level timeout happens to be, often minutes, and every
RPC call runs synchronously on the same thread as the UI. A five-minute
freeze and a fifteen-second one are both bad, but bounding it at all
was the smallest, lowest-risk piece of this whole set of changes.

**Non-blocking periodic refresh.** The real structural fix for the
freeze: with more than one server's window potentially open at once
(see the MDI work further up this file), one slow or unreachable server
freezing literally everything — every other window, all keyboard/mouse
input — on a periodic TIMER, not even in response to something just
clicked, was the sharpest edge of the whole synchronous-call design.
Rather than full OS threads (real complexity: tvision itself assumes
single-threaded access to every `TView`, so a background thread would
need its own result queue with its own synchronization, and touching
this codebase's already-careful "what happens if a client is destroyed
mid-request" handling — see the multi-server work's own entries above —
gets meaningfully harder once a second thread is involved), this uses
libcurl's own "multi" interface, still on the one UI thread: start a
request, then poll for completion on later ticks, never blocking. `App`
owns one shared `CURLM*` — one handle's own `curl_multi_perform()`/
`curl_multi_info_read()`, driven once per `idle()` tick, naturally
covers however many refreshes happen to be in flight at once, rather
than needing to ask every client's own separate multi handle
individually. `TransmissionClient` gained `startRefresh()`/
`finishRefresh()` alongside the existing synchronous `listTorrents()`
(kept for the initial fetch when a window first opens, and every other
action — each short, and a direct response to something just clicked,
unlike a periodic background refresh) — `CURLOPT_PRIVATE` set to the
requesting `TorrentListWindow*` when a request starts is how `App::idle()`
identifies which window's own `finishAsyncRefresh()` to call once
`curl_multi_info_read()` reports it done, without maintaining a separate
lookup of its own. A 409 (session renewal) during an async refresh
doesn't retry within the same request the way the synchronous path
does — it just stores the renewed session id and reports failure for
this cycle, relying on the next periodic tick to retry with it, which
avoids needing retry logic duplicated into the async state machine for
what's already a self-correcting, frequently-recurring call.

The one genuinely delicate part: a server can be removed (the
Connection dialog's own "[-]", now immediate — see its own entry
further up this file) while its own refresh is still in flight, and the
easy handle it's using MUST be detached from the shared `CURLM*` before
`TransmissionClient`'s destructor cleans it up — curl's own multi-handle
docs require every easy handle removed before the multi handle itself
is (or, in this app's case specifically, before the *client* owning
that easy handle is destroyed while the handle is still attached to
anything). Handled at two levels: the destructor itself detaches if
still in flight (covers a client destroyed for any reason, not just
this specific removal path), and `App`'s own destructor additionally
walks every window explicitly, canceling any in-flight refresh, before
cleaning up the shared multi handle itself — not left to whatever order
C++ happens to destroy `deskTop`'s own child windows in relative to
`App`'s own destructor body, which isn't something worth depending on
implicitly for something this easy to get subtly wrong.

**Network errors surfaced in the UI.** `TransmissionClient::lastError()`
existed already but was never actually shown anywhere — a known gap,
noted here once it became a much more frequent occurrence once
`call()` gained a real timeout (a request that used to hang
indefinitely now visibly fails within 15s instead). Also fixed
`lastError_` never being cleared on success, only ever set on failure —
meaning it could keep showing a stale error from an earlier failed
call even after a later one succeeded; every attempt now clears it
first, so `lastError().empty()` reliably means "the most recent attempt
succeeded". A messageBox popping up every single `refreshIntervalSeconds`
for as long as a server stays down would be far more disruptive than
the problem it's reporting, so instead: a small "(offline)" marker
appended to that server's own window title, the moment a refresh
attempt fails, cleared the moment one succeeds again — glanceable, and
gone entirely once the problem is.

Verified in layers, spending real effort on the one part that actually
carried risk rather than treating all four as equally safe. Persistent
connection: confirmed on a running instance across three different
call types on the same handle in a row (list refresh, start, a details
fetch) — all correct, nothing corrupted by option state carrying over
between them. But the real test was the removal race: a mock server
deliberately delaying its own `torrent-get` response by 5 seconds, a
2-second refresh interval, and timing the Connection dialog's own "[-]"
to land while a refresh was confirmed still in flight — removing the
server, then watching the process stay alive and fully responsive
through and past the exact moment the mock's delayed, now-orphaned
response would otherwise have arrived. As a side effect, the same test
also confirmed the non-blocking claim itself: the app kept responding
normally to keyboard/mouse input (opening the Connection dialog,
clicking "[-]") while that same slow request was still pending in the
background — the freeze this whole set of changes was meant to fix
simply didn't happen.

**Three changes at once: `TGridView`/`TGridWindow`/`TGridColumnManagerDialog`
moved into `tvision-ext/` alongside `TComboBox`, torrent-list windows
reverted from ordinary resizable MDI windows back to always filling the
whole desktop, and a new "Connections" menu for switching between
them.**

The file move itself was mechanical — every include across the app
updated from `../tgridview/...` to `../tvision-ext/...`, and (since the
three moved files all reference each other through same-directory,
relative includes already — `"TGridView.h"`, not a path — nothing
inside them needed to change at all). The one non-mechanical part: the
folder's own design-notes file, previously `tgridview/README.md`,
became `tvision-ext/TGridView-README.md` rather than a second plain
`README.md` sitting next to `TComboBox.h`'s own header-comment
documentation — a bare "README.md" would be ambiguous once the folder
holds more than one component's own writeup. Every in-code reference to
it updated to match, and `TGridView.h`'s own top-of-file comment (the
one explaining why this file has no project dependencies) now mentions
`TComboBox.h` alongside it, since it's making the same "reusable, could
be proposed upstream to tvision" case both files now share explicitly.

The window-sizing change is a genuine back-and-forth worth being
explicit about, not a straight line: originally one single window,
always locked to the whole desktop (`TGridWindow`'s own `fullScreen`
mode, built for exactly one such window ever existing). The MDI
multi-server work (see its own entry above) moved every server's window
to `fullScreen=false` instead — ordinary, resizable, tileable,
individually positioned — specifically so `Window → Tile/Cascade` could
usefully arrange more than one of them side by side, and gained its own
`closable=false` parameter on `TGridWindow` (see the entry after that
one) so they'd stay non-closable despite no longer being locked.
Asked for directly now: back to always exactly filling the desktop,
immovable, unresizable — one *per server*, stacked instead of tiled,
which `fullScreen=true` already supports without any change of its own
(it just wasn't exercised with more than one instance open at once
before). `TWindow`'s own default `growMode` (`gfGrowAll|gfGrowRel`, set
unconditionally in its own constructor) already keeps a window matching
its owner's bounds as they change — confirmed by reading tvision's own
`twindow.cpp` rather than assumed, since it's exactly what "adjusts
automatically when the terminal resizes" needs and nothing extra had to
be added for it. `closable=false` (added for the previous, `fullScreen=
false` phase) simply stops mattering with `fullScreen=true` — that mode
already excludes `wfClose` (along with `wfMove`/`wfGrow`/`wfZoom`) via
its own `flags=0` — so `TorrentListWindow`'s own constructor call
dropped that argument rather than passing a value that would go unused;
`TGridWindow`'s own `closable` parameter stays as-is for whichever
future ordinary MDI window wants it, on its own resizable path. Every
server's own saved window position/size (`AppSettings::windowLayouts`,
added specifically for the resizable phase) is genuinely dead weight
now that opening one always means the same one size regardless of
what's saved — removed entirely (`AppSettings`, `Config.cpp`, `App.cpp`
all together) rather than left in place unused, along with
`openServerWindow()`'s own now-pointless `bounds` parameter (always
`deskTop->getExtent()` at every call site once nothing else could ever
override it).

The "Connections" menu needed two things tvision doesn't obviously
support out of the box, and both got checked directly in its own
source before writing anything: rebuilding a menu's contents at
runtime, and making one specific item visually stand out from its
siblings. For the first: `TMenuItem`'s own fields (`name`, `command`,
`next`, and a submenu's own `subMenu`) are all public, and its
constructor copies the name string it's given rather than just storing
the pointer (confirmed in tvision's own `tmnuview.cpp`) — so a fresh
per-server item list can be built from ordinary local `std::string`s
and threaded onto the "Connections" `TSubMenu`'s own `TMenu::items`
safely, after manually freeing the old chain the same way `TMenu`'s own
destructor would (walking `next`, `delete`-ing each node) rather than
destroying and replacing the `TMenu` object itself. Getting a handle to
that one `TMenu` back later needed its own small trick: `initMenuBar()`
has to stay `static` (`TProgInit`'s own requirement), so there's no
`this` yet at the point it runs to hand the pointer to directly — a
single file-scope `TMenu*`, set once when the submenu is first built
and read from everywhere else that needs it, covers this safely since
exactly one `App` instance ever exists in this process. For the second:
`TMenuBox::draw()` computes one color for ALL items sharing a given
state (normal, selected, disabled — see its own source) — there's no
per-item font-weight or independent color in a text-mode menu without
writing a custom drawing override, so genuine "bold" wasn't literally
available. Repurposing the `~x~` markup a menu item's own name already
supports (normally wrapped around just one accelerator letter, to
underline it) around the *entire* label instead gives that whole item a
distinctly different, already-themed color for free — paired with a
bullet character, this is the closest a stock tvision menu gets to
"make this one item visually stand out" without a custom `TMenuBox`
subclass, and was a deliberate choice over building one just for this.
Bringing whichever server is picked to the front reuses
`openServerWindow()`'s own existing "already open → just `select()` it"
path unchanged (see the MDI work's own entry above) — nothing new
needed there, since every configured server's window already always
exists. Rebuilt from two different triggers, not one: from `idle()`
(catching a focus change from *any* cause at all — clicking a different
window, `Window → Next`, the Window List dialog, or this very menu —
the same "check what's true right now, every tick" shape
`updateBandwidthStatus()`/`cmManageColumns`'s own enable state already
use, since threading a callback through every single path that can
move focus individually would both miss something eventually and be
far more code) and directly from `showConnectionDialog()`'s own
`onServerSaved`/`onServerRemoved` callbacks (since the server *list*
itself changing isn't something a focus-change check would ever
notice on its own).

One real bug along the way, caught immediately by testing on a running
instance rather than trusting the code by inspection: the "Connections"
menu didn't show up in the bar at all at first. The top-level chain
built every other menu as siblings via bare `*new TSubMenu(...) +
*new TSubMenu(...) + ...` (resolving to `operator+(TSubMenu&,
TSubMenu&)`, confirmed by reading tvision's own `menu.cpp`), but this
one was written with the same `static_cast<TMenuItem&>(...)` idiom the
*Queue* submenu earlier in this same file genuinely needs — there,
deliberately forcing `operator+(TSubMenu&, TMenuItem&)` instead, so
Queue nests as an item *inside* the Torrent menu rather than becoming
another top-level sibling. Applying the same cast here forced the exact
opposite of what was wanted: Connections got silently nested inside the
*previous* top-level menu's own item chain instead of becoming a
sibling of its own. Removing the cast (letting the natural
`TSubMenu&`+`TSubMenu&` overload fire, matching every other top-level
entry already in the chain) fixed it immediately — confirmed visually,
the menu bar showing "Connections" between "Window" and "Columns" as
intended, `Alt+N` opening it. Also confirmed on a real running instance
with two configured servers: opening it showed a bullet on whichever
one had focus, picking the other one brought its own window to the
front (title bar and content both switching to match), and reopening
the menu afterward — with no explicit rebuild call anywhere in the
selection's own command handler, deliberately relying entirely on
`idle()`'s own generic check — showed the bullet correctly moved to
match, confirming that path (not just the explicit one from
`showConnectionDialog()`) genuinely works on its own. A fresh install
with no servers configured showed a single, disabled "Empty" entry as
intended.

**Four changes at once: renaming a file/folder, multi-selection
switched from press-and-hold to double-click, a middle-click shortcut
to a torrent's files window, and version 1.5.**

**Rename** is the only genuinely new feature of the four. `TransmissionClient`
gained `renamePath()` — its own RPC method (`torrent-rename-path`, not
`torrent-set` despite the similar name), because Transmission only ever
renames the *last* path component: it takes the item's current full
path and a new leaf name, not a new full path. `FileTreeRow` (the files
window's own per-row struct — see the multi-server work's own entries
further up this file for the general shape) gained a `path` field to
supply that full path: `emitTreeRows()` already recurses the folder
tree building `depth`/`fileIndices`/etc. for each row, so it picked up
building each row's own full path (parent's path + this row's own name)
along the way, at no real extra cost. Wired into the same right-click
context menu the wanted/priority actions already used, behind a new
"Rename" entry, using tvision's own `inputBox()` for the "what should
this be called" prompt (pre-filled with the current name) rather than
building a whole dialog by hand for one text field. On success, the
same full re-fetch every other action in this window already does
picks up everything the rename could have changed (a renamed folder's
own new path, every descendant file's own path if it was one, sort
order shifting if the new name reorders it among its siblings) from
the same source of truth as the rest of the window, rather than trying
to patch `files_`/`rows_` in place for just this one case.

**Multi-selection's entry gesture changed from a press-and-hold to a
double-click, with double-click also being what exits it again** —
asked for directly, not a bug. The old long-press watched the mouse in
a blocking loop (`watchForLongPress()`, removed entirely along with the
`kLongPressMs` constant) for up to 800ms before deciding whether to
enter selection mode; a double-click makes the same decision
immediately, and being the same gesture both ways ("double-click to
start, double-click to stop") is easier to discover and remember than
a gesture with no obvious mirror to undo it. The double-click-enters
path shares its own event with `CellActivateFn` (e.g. the Queue
Position column's own cycling) — that callback still gets first
refusal, exactly as `RowActivateFn` used to before this dialog-opening
its own case, only entering selection mode once it declines. The
double-click-exits path reuses what used to just be a no-op: the
second `mouseDown` of a double-click already arrived flagged
`meDoubleClick` while in selection mode, previously skipped there so it
wouldn't toggle the row twice and silently cancel itself back to the
original state — now it calls `exitSelectionMode()` instead of doing
nothing, since "the same click would have undone itself anyway" turned
out to be the perfect signal for "this means leave", not just a case to
ignore.

**Middle-click on the main torrent list jumps straight to that
torrent's files window** — a new `RowMiddleClickFn` callback on
`TGridView` (mirroring `RowContextFn`'s own shape and, for the same
reason documented on that one, has to be handled before
`TListViewer::handleEvent()` ever sees the event: its own click-
tracking loop doesn't check which button was pressed either).
Deliberately doesn't touch right-click's own context menu at all —
asked for explicitly, after an initial version of this same request
would have replaced it instead; middle-click is a genuinely separate,
additive gesture, not a substitute for anything already there.

Verified each piece where it could actually be verified with
confidence rather than only where it was easiest to: the double-click
selection-mode toggle and the middle-click shortcut both confirmed on a
real running instance (checkbox column appearing/disappearing across
two double-clicks; a real files window opening, its own buttons and
scrollbar visible, after a middle-click). Rename's own path-computation
logic — genuinely new code, not a rewire of something already proven —
got a dedicated, isolated test instead: the exact tree-building
functions this file uses (copied into a standalone harness the same
way `closeWindowsForClient()` got tested a few entries up this file,
since they live in an anonymous namespace not reachable from outside
it), fed a torrent with files nested three folders deep, confirming
every row's own computed path exactly matched what `torrent-rename-path`
would need for it — a folder three levels down and the file beneath it
alike. Right-click delivering the "Rename" menu item itself wasn't
re-verified beyond that: the mechanism is unmodified, existing code
(`showContextMenuFor()`/`setRowContextCallback()`), already proven
working by the wanted/priority actions already using the exact same
menu.

**Torrent-list column widths/order/visibility became per-server,
having stayed one shared setting since the MDI work made that stop
making sense.** Reported directly: resizing or reconfiguring one
server's window was silently overwriting every other server's own
column layout too, since `AppSettings` still had exactly one
`columnWidths`/`columnOrder`/`columnVisible` triplet — left that way
deliberately at the time ("first step" scope, see the MDI work's own
entry above), but a real gap once every server got its own
independently-managed window.

`AppSettings::ColumnLayout` bundles the three together (they're always
edited as a unit — dragging a separator, double-clicking a header,
"Manage columns...") and `columnLayouts` keys one per server name, the
same shape `windowLayouts` already used for position/size. Nothing
about `TorrentListWindow` itself needed changing — it already took
these three as per-*window* constructor parameters (`initialColumnWidths`
and friends), which is exactly the right scope; the bug was entirely in
how `App` was filling them in, from one shared field instead of a
lookup keyed by the window's own server name. Three call sites needed
that lookup instead of the flat field: `openServerWindow()` (read, when
a window is created or reopened), `showColumnManagerDialog()` (write,
the instant that dialog confirms a change — matches the existing
"visibility is a discrete choice, not a continuous drag" distinction
`ColumnLayout`'s own fields already draw), and `shutDown()` (write,
every still-open window's own current widths/order, the same way it
already did for position/size — previously only captured window[0]'s
own layout and wrote it under the one shared field, silently discarding
every other open window's own settings on every exit). Sort
column/direction stayed as it was — genuinely still one shared setting
across every window, not reported as part of this, and not obviously
the same kind of "per-server" question the other three are (there's no
single UI action producing it the same "customize one window" way
resizing or reordering a column does).

Verified with a direct check on the read side rather than only the
write side, since a get-then-redisplay bug (loading the right data but
still somehow rendering it the same everywhere) would have looked
identical to a correctly-scoped one on a quick glance: a `settings.json`
with a custom, distinctive column width saved only under one server's
own name, loaded through the real `Config.cpp`, and two windows built
the way `openServerWindow()` builds them — the one for that server
came back with the custom width; the other, with no entry of its own,
came back with the untouched defaults. Caught a mismatch in the test
itself along the way, not the implementation: an initial attempt fed a
7-element width array (one per *visible* column) into a mechanism that
actually expects one entry per column *including hidden ones* (16
total) — already documented behavior (a mismatched count falls back to
defaults, exactly as intended for an older `settings.json` predating a
column being added), but it meant the first pass showed both windows
identically wrong for a reason that had nothing to do with the
per-server logic being tested. Fixed the test, not the code, and reran
with a correctly-sized array to get the real answer.

**Connection dialog: host/port/user/password disabled until the shown
server name is actually a registered entry.** Asked for as a design on
top of the Save/OK state machine just above (deliberately built to
complement it, not conflict — the two share the same underlying
question, "does the name shown right now correspond to something
real?", just answering it for two different things: whether to reset
the fields to defaults or load real data, and now, separately, whether
those fields should even be touchable at all).

"Registered" is deliberately a different question from "saved" — a
name just added via "[+]" is real enough to configure (it's sitting
right there in the combo's own list) even though nothing's been Saved
under it yet, so it needed its own check (`TComboBox::allValues()`,
searched for the currently-shown name) rather than reusing the
existing "is this in `settings_.servers`" test the load-vs-defaults
logic already had. A name that's neither registered nor saved gets
both: reset fields *and* disabled ones — the two decisions computed
side by side in the same broadcast handler, from the same two checks,
so they can never disagree about a given name.

`setState(sfDisabled, ...)` turned out to need nothing else alongside
it — confirmed by reading `tgroup.cpp` rather than assumed: disabled
views are already skipped entirely by `focusNext()` (Tab won't land on
one) and by the event-dispatch helper that delivers positional/focused
events (disabled views never receive mouse clicks or keystrokes at
all), both core to how every view in tvision already behaves. Nothing
project-specific needed adding on top for either half.

One gap surfaced in `TComboBox` itself along the way: `"[-]"` removing
the very last remaining entry left the list empty without broadcasting
`cmComboBoxSelectionChanged` — the emptying-out branch predates the
broadcast being centralized into `focusItem()` (see the entry above),
and didn't get a manual fire added when everything else did, since at
the time nothing needed to know about *emptying out* specifically, only
about focusing a different entry. Fixed by firing it there too, since
skipping it would have meant this dialog's own fields staying
enabled — showing whatever the just-removed server's own details still
were — after removing the last configured name entirely. Fixed at the
source in `TComboBox` rather than worked around in the dialog, for the
same reason the broadcast was centralized into `focusItem()` in the
first place: a future caller relying on this broadcast to mean "the
shown value changed, no exceptions" shouldn't have to discover the
empty-list case is one.

Verified both directions on a real running instance rather than
assumed from the `setState()` call alone: with no server configured yet
and the combo showing its own placeholder name (not a registered
entry), Tab was sent repeatedly with a character typed after each — it
never reached host, port, user, or password, all four staying at their
untouched defaults throughout, confirming they were both invisible to
keyboard focus and inert against direct input. Clicking "[+]" to
actually register that name, then repeating the same Tab-and-type
sequence, showed the typed character land in the host field exactly as
expected — confirming the fields didn't just start disabled, but
genuinely re-enable the moment a name becomes real.

**Connection dialog: a real Save/OK state machine, and resetting the
connection fields to defaults on an unmatched server name.** Asked for
directly as a design, not found as a bug — worth documenting the
reasoning as thoroughly as the code itself, since several pieces had to
line up.

The defaults-reset needed `TComboBox`'s own broadcast to widen. It used
to fire `cmComboBoxSelectionChanged` only from the dropdown-pick path,
inline in `openDropdown()`. Moved into `focusItem()` itself instead —
the one place every path that changes what's focused already goes
through (dropdown pick, "[+]", "[-]", `newList()`), so nothing has to
remember to fire it separately (this class had exactly that kind of bug
once before — see the editBuf_-syncing comment already in `TComboBox.h`
— so consolidating into the one function that's already the accepted
place to keep this in sync was the deliberate choice here, not just a
convenience). Typing itself doesn't call `focusItem()` at all, so the
keystroke-handling paths (insert, backspace, delete — not plain cursor
movement) needed their own explicit fire alongside it. `message()`
itself no-ops on a null receiver (confirmed in tvision's own
`misc.cpp`), so firing this from inside `focusItem()` — including
during construction, before this view has an owner — never needed a
null check of its own.

`ConnectionDialogImpl`'s handler for that broadcast now does one of two
things depending on whether the shown name matches an entry in
`settings_.servers`: loads that server's own saved profile (unchanged
from before), or resets to a default-constructed `ServerProfile` (the
same 127.0.0.1:9091, no user/password every `AppSettings::servers`
entry would start from) — rather than leaving whatever the previously-
focused server's own details happened to be, which was the actual
report: switching or typing a new name kept showing an unrelated
server's host/port as if they belonged to the new one.

The Save/OK button needed to stop being the fixed cmOK-labeled button
it always was. `ConnectionDialogImpl` gained a `dirty_` flag and a
`setDirty()` that both updates it and relabels the button (`TButton::
title`, reassigned with the same `newStr()`/`delete[]` convention
already used for `TWindow::title` elsewhere in this project) — kept as
the one place that touches either, so the two can't drift apart. Dirty
starts true unless the initially-shown server already matches a saved
profile exactly. It's set false only right after Save's own connection
test actually succeeds; set true again by the combo's own broadcast
(covering typing, dropdown picks, and "[+]"/"[-]" alike, now that it's
unified — see above) AND, separately, by a raw `evKeyDown` reaching any
of host/port/user/password directly (captured via `current`, this
dialog's own inherited `TGroup::current`, read *before* dispatching the
event down to whichever field has focus — reading it after can no
longer be trusted, since `TDialog::handleEvent()` may have already
moved focus on its own by then, e.g. via Tab). The button's own click
handler branches on `dirty_`: false skips straight to `endModal(cmOK)`
(Save already did everything last time); true runs the connection test,
and only past that — the real save, via the new `ServerSavedCallback`
(see below) — then `addCurrentValue()` to make sure the combo's own
list has this name as a real entry even if it was typed and Saved
directly without ever clicking "[+]" first. Deliberately in that order,
save-then-sync: `addCurrentValue()` can itself show a "Server added"
confirmation (the same one "[+]" shows) the first time a brand new name
reaches this point, and the actual persistence shouldn't depend on
where that confirmation happens to land.

`App::showConnectionDialog()`'s side needed a new `ServerSavedCallback`
alongside the existing `ServerRemovedCallback`, called the instant Save
succeeds — not deferred to whenever (or if) this dialog eventually
closes with cmOK, since Save deliberately keeps it open. It persists
that one server's profile to `settings_`, and opens or updates its own
window, mirroring `onServerRemoved`'s own "act immediately, don't wait
for OK/Cancel" shape. With server data now handled entirely through
these two callbacks, the code that used to run after `execView(dlg) ==
cmOK` shrank to just refresh interval and language — anything server-
related reaching that point would already be redundant, since Save
already did it the moment the button last read "OK".

Verified in layers, several with a synthetic harness driving
`ConnectionDialogImpl::handleEvent()` directly rather than through a
live interactive session — precise for exactly the kind of "did this
one internal flag change" questions involved here, in a way clicking
through a real dialog wouldn't be. A server already saved with a dead
port, edited to a live one and Saved: the callback fired with the
*edited* values, not the original ones, confirming the test genuinely
used what was currently in the fields rather than something cached.
The very next click, unchanged, fired nothing further — confirming
`dirty_` correctly gated a second, redundant test+save once the button
read OK. A saved server's fields shown, then the name switched to an
unmatched one and back: confirmed the fields actually reset to generic
defaults on the way out and reloaded the real saved values on the way
back, not stale data drifting between the two. One genuine surprise
along the way: the first version of this test appeared to hang — not a
bug, but Save's own `addCurrentValue()` correctly reaching a real,
correctly-blocking `messageBox()` (the "Server added" confirmation) for
a brand new name with nowhere for a synthetic single-shot test to send
a dismissal — resolved by reordering the real save ahead of that
cosmetic list-sync in the implementation itself (described above) and,
for the test, choosing scenarios that exercise an *existing* entry
(where `addCurrentValue()` finds a match and skips the confirmation
entirely) to observe the rest of the state machine without that
particular blocking step in the way.

**"[-]" removing a server didn't actually close anything unless OK was
also pressed afterward — and pressing Cancel instead quietly undid a
removal its own confirmation popup had already told the user had
happened.** A real follow-up to the fix just below this one (which got
the *closing itself* right — every window pointing at the removed
server's client, not just its torrent-list window — but still only
ever ran inside the `execView(dlg) == cmOK` branch, gated behind
however this dialog happened to close). Asked directly rather than
found by testing: does Cancel actually undo it? It did, and shouldn't
have — "[-]" needed to stop behaving like every other field in this
dialog (nothing takes effect until OK) and start being immediate,
matching what its own confirmation popup already implied.

`createConnectionDialog()` gained a `ServerRemovedCallback` parameter —
a `std::function<void(const std::string&)>` `App::showConnectionDialog()`
supplies, called the instant "[-]" actually removes an entry (not the
no-op cases), from inside `ConnectionDialogImpl::handleEvent()` itself,
well before the dialog's own `execView()` call returns with anything at
all. The callback does the *entire* removal right there — closing every
window for that server (the same `closeWindowsForClient()` plus its own
torrent-list window from the fix below), dropping it from `settings_.
servers`, and `saveSettings()` immediately — not deferred to whatever
`connectionDialogResult()` would otherwise do on OK. That function's own
loop rebuilding `result.servers` from the combo's current list stayed
in place and needed no changes: by the time OK (if it's even pressed)
reaches it, the removed name is already gone from both the combo's own
`allValues()` and `settings_.servers` itself, so there's nothing left
for it to redundantly remove — the dead code that used to do that
inside the `cmOK` branch was deleted rather than left stale alongside
the new immediate path.

Verified against the exact sequence that exposed the gap: a torrent's
Details window opened, then Connection, then "[-]" on the one
configured server, confirming its own popup — at which point the
torrent-list window *and* the Details window were both already gone
from the screen, before touching the Connection dialog's own OK or
Cancel at all. Pressing Cancel afterward changed nothing further (both
windows stayed gone), and `settings.json` already had an empty `servers`
object — confirming the removal was real and permanent from the moment
"[-]" was confirmed, not something Cancel could still walk back.

**Removing a server via the Connection dialog's "[-]" didn't actually
close anything.** Two gaps, found together: the server's own torrent-
list window wasn't reliably closing, and even where it did, any
Details/Files/Tracker window still open for one of that server's
torrents was left behind entirely — each holding a `TransmissionClient&`
reference to a client `App` was about to erase, which would have been
a genuine dangling-reference hazard the next time any of them tried to
refresh or apply a change, not just a leftover window cluttering the
desktop.

`TorrentDetailsWindow` already had exactly the right tool for this,
unused: a `clientPtr()` accessor and a comment describing precisely
this scenario — apparently added ahead of actually needing it, then
never wired up on the `App` side. `TorrentFilesWindow` and
`TrackerListWindow` got the same accessor added to match (`TrackerDetailWindow`
didn't: it holds no `TransmissionClient` reference at all, just a
`TrackerStat` snapshot passed by value, so it isn't a safety hazard —
left open when its server is removed, if one happens to be, showing
whatever it already had rather than closing along with the rest).
`App` gained `closeWindowsForClient()`, matching every other "find
this on the desktop instead of caching a pointer" helper already in
this file: walks every window, closes whichever of the three types
above points at the client being removed. `showConnectionDialog()`
now calls it, in the order that actually matters — every other window
first (while the client is still alive, so `clientPtr()` still has
something valid to compare against), then the torrent-list window
itself, and only then the client is erased.

Verified the mechanism directly: three windows (Details, Files,
Tracker) built against one client, a fourth Details window built
against a completely different one, confirmed all four exist, then
confirmed closing "for" the first client took out exactly those three
and left the fourth alone — the actual risk here was ever closing the
wrong one (or missing one), not whether `close()` itself works.

**Two follow-ups to the MDI multi-server work just above, both real
gaps in it rather than new features.**

**Start/Stop and the rest of the Torrent menu would briefly show the
right state after switching focus, then drift to some other window's
state moments later.** Reported precisely: consistent right after
switching, wrong "a few instants" afterward, as if something on a
timer kept moving the goalposts — which is exactly what was happening.
`updateCommandStates()`'s own `setState()`-triggered fix (see further
up this file) only handled *changing which window has focus*; it never
accounted for `TorrentListWindow::refresh()` (via
`applyFilterAndSort()`) calling the exact same method unconditionally
on every window, focused or not — and `App::idle()` refreshes every
open window on one shared timer, not just the focused one.
`enableCommand`/`disableCommand` are process-wide, not per-window, so
an unfocused window's own periodic refresh would silently overwrite
whatever the focused window's own refresh (or its `setState()`) had
just set, with its own selected torrent's state instead. Fixed at the
source rather than in every caller: `updateCommandStates()` itself now
returns immediately unless `state & sfActive` — the window pushing
values into that shared state has to actually be the current one, no
matter which of its own code paths got it there. Verified with the
scenario that exposed it in the first place: two mock servers, one
stopped (Start should stay enabled) and given focus, the other
downloading (Start should stay disabled) and left to refresh
unattended on a 1-second interval for several cycles — then confirmed
Start was *still* enabled by pressing it and checking the RPC actually
reached the stopped one, not silently dropped.

**Every per-server window could be closed from the window itself,
which shouldn't be possible** — every configured server is meant to
always have one open; the only real "close" is the Connection dialog's
own "[-]" removing the server entirely (see the multi-server work
above), not something the window's own close box should also do.
`TGridWindow` (the generic, reusable base this sits on — deliberately
left alone otherwise, along with `TGridView` underneath it, so this
stays a base-level, opt-in flag rather than special-cased for one
caller) gained a `closable` parameter, independent of `fullScreen`:
the two used to be conflated (`fullScreen=true` implied *and only ever
meant* non-closable, `fullScreen=false` implied *and only ever meant*
closable), but an MDI window that shouldn't be user-closable is exactly
what this app's own torrent-list windows needed and neither existing
combination covered. `TorrentListWindow` now passes `closable=false`
alongside its existing `fullScreen=false` — confirmed
`TWindow::close()` itself only checks `valid(cmClose)`, not `wfClose`,
so the *programmatic* close the Connection dialog's own "[-]" still
needs (see `App::showConnectionDialog()`) keeps working exactly as
before; only the *user-facing* close box and Alt+F3 go away, gated by
`wfClose` alone. Verified visually (no close box rendered where one
used to be) and functionally (clicking that exact spot, and the
process staying alive with the window still there, rather than only
checking that a click compiled without incident).

**Turned the torrent list into one MDI window per configured server.**
The single biggest architectural change so far — every server named in
the Connection dialog now gets its own always-open torrent-list window,
titled with that server's own name, rather than one shared window
whose connection changed underneath it.

The underlying pieces mostly already existed. `TGridWindow` (the base
this window sits on) already had a `fullScreen` parameter switching
between "locked, always-maximized" and "ordinary MDI child window
(movable, resizable, closable, zoomable)" — the torrent list had just
never actually been built with `fullScreen=false`. `App` already had a
`focusedGrid()` helper for "Manage columns..." to act on whichever
grid-based window has focus; the same idea generalizes to
`focusedListWindow()`/`allListWindows()` for every Torrent-menu action.
`App` also already searched the desktop directly for tracker windows
rather than caching a pointer to them (see `shutDown()`'s own tracker
backstop) — turned out to be exactly the right instinct to follow here
too, for a concrete reason: `fullScreen=false` means `wfClose` is now
set, and `TWindow::close()` destroys the window outright — a cached
`TorrentListWindow*` held across event-loop turns (the way the old
single `listWindow_` member was) would go dangling the moment its
window was closed. Nothing in `App` keeps one anymore; every action
looks a window up fresh from the desktop's own child chain at the
moment it's needed.

`AppSettings`'s single `host`/`port`/`user`/`password` (already
replaced by the `servers` map — see the multi-server work further up
this file) gained a `windowLayouts` map (position/size, keyed by server
name) and a `focusedServerAtClose` string, saved in `shutDown()` and
restored in the constructor — clamped against the *current* terminal
size on restore (`TRect::intersect()`), since a saved layout from a
larger terminal would otherwise open something bigger than what's
actually there, or an offscreen sliver if the saved position doesn't
fit at all; falls back to the full desktop extent if what's left after
clamping is too small to be useful.

Two real bugs surfaced along the way, neither exotic once found, both
only mattering because there's now more than one of these windows:

- `TGridWindow` set `ofTileable` (required for `TDeskTop::tile()`/
  `cascade()` to include a window at all — confirmed by reading
  `tdesktop.cpp`, not assumed from the name) only on the `fullScreen`
  branch — backwards from what actually needed it, since a `flags=0`
  fullscreen window being tileable would risk `tile()`/`cascade()`
  elsewhere on the desktop forcibly repositioning it via `locate()`
  (which doesn't check `wfMove`/`wfGrow` — those only gate *interactive*
  move/resize). Moved to the non-fullscreen branch, where it belongs.
- `TorrentListWindow::updateCommandStates()` (enables/disables Start,
  Stop, and the rest of the Torrent menu based on the selected torrent)
  only ever ran from the grid's own row-focus callback. `enableCommand`/
  `disableCommand` are process-wide, not per-window — so a window that
  gained focus *without* also changing its own row focus (the usual
  trigger) would silently keep showing whichever other window's state
  was set last. Fixed with a `setState()` override reacting to
  `sfActive` turning on (confirmed via `twindow.cpp`/`tgroup.cpp` that
  this is exactly the flag a window's own gain-of-focus sets) — the
  same "override setState for a redraw/resync tvision doesn't trigger
  on its own" pattern already used by `TComboBox` and `TCheckBoxes`-like
  controls elsewhere.

Verified in layers, each isolating a different piece rather than one
big end-to-end pass: two mock servers with distinguishable torrents
confirmed separate windows show separate data (not one leaking into
the other) and correct per-server titles; `Window → Tile` confirmed
both windows actually exist and get arranged (not just the newest one
visible on top of an identical stack); F5 (Start) with a mock tracking
which server received the RPC call confirmed the action follows
whichever window has focus, re-checked after switching focus by mouse
click; closing the focused window (clicking its own close box)
confirmed the app stays alive and focus falls back to what's left,
directly addressing the dangling-pointer risk above rather than just
hoping it doesn't happen; a full close/reopen cycle against a real
`settings.json` confirmed both windows' positions (post-Tile) and which
one had focus came back exactly as saved. The `updateCommandStates()`
fix specifically was checked with a small standalone harness rather
than through the interactive TUI — flipping a command's state by hand,
then calling `setState(sfActive, True)` on the *other* window directly
and confirming the shared command state changed to match it — precise
and immediate in a way that clicking through a live session, already
fragile for pinpointing exact window regions in earlier sessions (see
elsewhere in this file), wouldn't have been for this specific question.

**Connection dialog: confirmation popups for "[+]"/"[-]", and a
connection test before OK actually saves anything.** Two additions to
the multi-server work just above.

The confirmations needed `TComboBox` itself to say a *little* more than
it used to: `addCurrentValue()`/`removeCurrentValue()` already knew
when they'd actually changed the list versus done nothing (an already-
present name for "[+]", no match for "[-]" — see their own comments),
but had no way to tell a caller *what* changed, since `message()`'s own
broadcast only carries a `void*` back, not a string. Two new commands
(`cmComboBoxItemAdded`/`cmComboBoxItemRemoved`, 60/61 — the next free
values after `cmComboBoxSelectionChanged`'s own 59, picked to stay
clear of both tvision's reserved low range and this project's own
`cmUserBase`-area commands starting at 100) fire only on an actual
change, alongside a new `lastChangedValue()` accessor a caller reads
from inside its own broadcast handler — set right before the item is
actually added or (for removal) right before it's freed, so it's never
stale by the time anything asks. `ConnectionDialogImpl` (already
catching one broadcast, for auto-populating fields on selection change)
just gained two more cases in the same handler.

The connection test needed the OK button itself to stop being a plain
`cmOK` button — tvision's own `TButton`/`TDialog` machinery treats
`cmOK` (along with `cmCancel`/`cmYes`/`cmNo`) specially, validating
every field and calling `endModal()` directly the moment it's clicked,
before this dialog's own `handleEvent()` ever gets a look at it. A new
local command (`cmTestConnectionAndOK`, 62 — grouped with the two
above) lets the dialog intercept the click itself: run every field's
own validator first (`valid(cmOK)` — the argument only matters for
telling `cmCancel` apart from everything else, which skips validation
entirely; it's not literally what happens next), then a throwaway
`TransmissionClient` built straight from whatever's currently in the
fields (not the app's own shared one, which stays pointed at the
*previous* connection until this one actually succeeds) makes the same
kind of `session-get` round trip "Server Configuration" already relies
on for its own fetch. Only on success does the handler call
`endModal(cmOK)` itself; a failure shows the error and leaves the
dialog open, `connectionDialogResult()` (and so any actual save) never
reached at all.

Verified without needing to click through an actual interactive
message box for any of this — the box blocking, once reached, is
already the confirmation something got that far, so a synthetic
`handleEvent()` call left waiting past a short timeout is treated as a
pass, not a hang to work around: sending `cmTestConnectionAndOK`
against a host nothing was listening on left the call blocked past that
timeout (the error box, reached and waiting); the identical call
against a real mock server returned immediately (no box — the success
path, `endModal(cmOK)`, taken instead); and a synthetic click on "[+]"
left the call blocked the same way (the confirmation, reached and
waiting) rather than returning immediately.

**`TComboBox` gained an editable mode, and the Connection dialog now
manages more than one named server with it.** Two pieces, built and
verified in that order.

The widget change first: `setEditable(true)` turns the box from a
pure picker into something that can also be typed into directly, with
"[+]"/"[-]" buttons appended before the dropdown arrow to add/remove
whatever's currently shown as a list entry. Deliberately a minimal text
editor rather than a full `TInputLine` port — single-line ASCII,
clipped rather than scrolled when the text is wider than the box, no
selection or clipboard (see the class's own header comment for the
reasoning) — enough for what it was built for (typing a short name).
One design choice worth calling out: `focusItem()` (called from
picking an entry in the dropdown, "[+]", "[-]", and `newList()` alike)
is the single place that syncs the edit buffer to whatever's now
focused, rather than each caller doing it themselves — the alternative
(each of those four call sites remembering to sync it) is exactly the
kind of thing that stays correct for a while and then silently doesn't
once a fifth call site is added later without the same care.
`allValues()` reads back the full list in order, for a caller that
wants to persist whatever the user has built up via "[+]"/"[-]"
somewhere.

`AppSettings`' own flat `host`/`port`/`user`/`password` fields became a
`std::map<std::string, ServerProfile>` (`servers`) plus `activeServer`,
with an `activeProfile()` accessor so `App`'s constructor, `applySettings()`,
and the CLI (`Cli.cpp`) didn't each need their own "which server" logic
— they already all just wanted "the currently active one's details".
Deliberately not migrated from the old shape: an older `settings.json`
that still has flat `host`/`port`/etc. simply doesn't populate
`servers` at all once loaded (nothing in `loadSettings()` looks at
those keys anymore), which in practice means starting over with an
empty server list — asked for explicitly rather than assumed, given how
early this project still is.

The Connection dialog itself needed one more thing beyond just adding
the combo: reacting when the user picks a *different* saved server from
the dropdown, by loading that server's own details into the other
fields automatically (typing a brand new name, not yet saved, leaves
the fields alone instead — see `TComboBox::cmComboBoxSelectionChanged`'s
own broadcast). That needed a small `TDialog` subclass local to
`ConnectionDialog.cpp` to catch the broadcast, rather than the plain
`TDialog` this dialog used to be built from directly. On confirming
with OK, the combo's own current entry list (not the settings passed
in) decides which saved servers survive into the result — a name
removed via "[-]" needs to actually disappear from what gets saved, and
only what's still in the combo (not what's missing from the old
settings) can tell that.

Verified in layers again: the combo's own edit-mode mechanics in
isolation first — typing, backspace, "[+]" (no duplicate created when
the current text already matches an entry), "[-]" (a no-op, not a
crash, when the current text matches nothing) — each confirmed via
direct event injection rather than a live interactive session (faster
and more precise for this kind of check, and this project already has
plenty of live-session coverage elsewhere in this file for whether
clicking a button in a real running dialog actually reaches it). Then
the full flow through `ConnectionDialog` itself: a fresh install
showing no servers and a placeholder name; confirming one populates
`servers` and `activeServer` correctly; reopening the dialog against
that result starts the combo on the right name with the right list;
adding a second server via "[+]" and confirming preserves the first
one's own details untouched while saving the second's; and — the
broadcast-driven auto-populate specifically — selecting a previously-
saved name back on a dialog opened with a *different* one active
correctly refills host/user/etc. from that server's own saved profile.
Finally the actual file round trip: `saveSettings()`/`loadSettings()`
against a real `settings.json`, confirming both servers' fields
(including the password surviving obfuscation and back) and
`activeServer` came back exactly as saved.

**Vendored TComboBox into the project**, at the request of wanting to
keep evolving it without every change going through the tvision fork
first. Combined the four classes involved — `TComboItem`,
`TComboViewer`, `TComboWindow`, `TComboBox` — spread across the fork's
own `dialogs.h` (declarations) and three separate `.cpp` files
(`tcombobo.cpp`/`tcmbovie.cpp`/`tcmbowin.cpp`) into a single
`src/tvision-ext/TComboBox.h`+`.cpp` pair, using an ordinary
`#pragma once` rather than tvision's own `Uses_X`/`__TComboBox`
include-guard convention — those classes are no longer declared
anywhere behind `Uses_TComboBox` at all now, so nothing needs that
macro defined to reach them. `LanguageComboBox.h` (the only thing in
this project that used to) now includes the new header directly. One
piece stayed as a dependency on the fork rather than getting copied in:
`cmComboBoxSelectionChanged` itself, which the fork added directly to
`views.h`'s own command enum unconditionally — not gated behind
`Uses_TComboBox` — so redeclaring it here collided outright rather than
needing vendoring; noted in the header's own comment rather than left
unexplained. This project's `external/tvision` submodule still points
at the same fork — whether anything else there still differs from
upstream in a way this project actually depends on hasn't been
re-audited as part of this change (see "Building" above); this was
specifically about combo box work no longer needing to go through the
fork.

Verified the vendored version behaves identically to the fork's own,
not just that it compiles: opened the popup on a real running instance,
confirmed all five languages render with a working scrollbar, picked a
different one, and confirmed the combo box's own displayed text updated
to match — the same round trip `LanguageComboBox` has always relied on,
now running entirely on this project's own copy of the code.

**Added the "Speed Limit" mode's own on/off checkbox to Server
Configuration**, deliberately left out when the screenshot that
prompted the dialog split (see further down this file) didn't show
one — turned out that omission was worth reconsidering rather than
assuming the screenshot was the last word on it. Wired through exactly
the same way every other field on that dialog already was: `SessionLimits`
gained `altSpeedEnabled`, read from and written straight to
`alt-speed-enabled` via `session-get`/`session-set` — asked for
explicitly as a reminder that these fields are live daemon state, never
something to cache into this app's own settings.json and reload from
there instead. That reminder matched what the code already did (nothing
on this dialog was ever being saved locally to begin with), so nothing
needed correcting on that front — but it's exactly the assumption worth
double-checking before adding anything new to this particular dialog,
since getting it wrong here specifically would mean silently showing
stale state instead of what the server actually has.

Verified with a mock server as before: the checkbox opened checked
against a mocked `alt-speed-enabled: true`, confirmed on a real rendered
screen, and confirmed `alt-speed-enabled: true` was among the fields
sent back in `session-set` after clicking OK — the same live round trip
already relied on for the limits sitting right next to it.

**Connection dialog field order and spacing**, tweaked after seeing it
rendered: Language moved to the top (above Refresh, where it used to
sit last, right before the buttons), and the dialog grew by one row so
there's a blank line above OK/Cancel instead of them sitting right
under Password. Confirmed off a real rendered capture rather than
trusting the coordinate math alone — worth a specific note since a
first attempt at capturing it looked like the lower half of the dialog
(User, Password, the buttons) wasn't rendering at all, which turned out
to be an incomplete read in the test itself: a single blind `os.read()`
some time after the keypress doesn't guarantee every byte the terminal
sent has actually arrived by then, since output can land in more than
one chunk. Draining the pty with `select()` until nothing new arrives
for a short stretch, rather than one fixed-size read after a fixed
delay, showed the dialog was actually complete and correctly laid out
the whole time.

**Split the combined Settings dialog into Connection and Server
Configuration**, matching a screenshot from the official Transmission
Android app showing global speed limits alongside a "Speed Limit" mode
section this app didn't have yet. The two dialogs' fields genuinely
live in different places — Connection's in this app's own
settings.json, Server Configuration's on the Transmission daemon itself
via `session-get`/`session-set` — so the split also removed the one
case where opening a single combined dialog needed a live RPC call just
to show fields that had nothing to do with the connection being
configured. `SessionLimits` gained `altSpeedDown`/`altSpeedUp`
alongside the existing global limit fields, read/written via
`alt-speed-down`/`alt-speed-up`; no `alt-speed-enabled` field was added
anywhere, matching the screenshot itself — enabling that mode is a
toggle that lives elsewhere in the real Transmission clients, not
something this particular dialog exposes.

Verified with the same care the original combined dialog's own field
pointers got (see "Settings dialog fields silently swapped" further
down this file, the reason both dialogs return direct field pointers
rather than scanning for them after the fact): Connection's own
pre-fill and result-extraction logic checked directly (construct with a
known host, confirm the field shows it, change it, confirm the
extracted result reflects the change) rather than through the
interactive dialog itself — closing a `TDialog` via its OK button is
generic tvision behavior already exercised throughout this app, not
new code worth re-proving through a fragile simulated click. Server
Configuration's fetch-populate-confirm-send round trip *was* worth
checking end to end, since the RPC shape itself changed: a mock
tracking `session-set` calls confirmed the dialog opened pre-filled
with the mock's own values (including the two new alt-speed fields),
and that clicking OK sent back exactly those values, unmodified,
alt-speed included. Along the way, ran into a case worth noting for any
future interactive dialog test: a `select()`-free `os.read()` on the
pty blocks indefinitely once a click stops producing *new* terminal
output, even when the app itself is behaving completely normally (just
not drawing anything new right that instant) — not a hang in the app,
a hazard in the test technique itself, worth guarding against
proactively rather than only after hitting it.

**Queue reordering**, added the same three ways priority already works
in the files window: a cycling double-click, a menu with the specific
choices, and the same on the right-click context menu. Two things
specific to this one, beyond just repeating that pattern:

`CellActivateFn` (see the files window's own priority cycling) needed a
real change, not just a new use: it used to always let the event fall
through to `TListViewer::handleEvent()` afterward regardless, on the
reasoning that a pre-flagged double-click never blocks there anyway
(true, and still true) — but the main list already has `RowActivateFn`
wired up for opening details on double-click, for every column, and
letting that still fire *in addition to* the queue column's own cycling
would mean double-clicking the queue column both cycled it and popped
open a details window every time — not what anyone would want.
`CellActivateFn` now returns whether it considered the double-click its
own to handle; true consumes the event outright (skipping
`RowActivateFn` for that one double-click), false leaves it alone
exactly as before. The files window's own callback just always returns
true (it never had `RowActivateFn` set to begin with, so nothing
changes there); the main list's returns true only for the queue column,
false for every other one, so details still open normally everywhere
else.

Nesting a submenu (the menu bar's "Torrent" needed to contain "Queue",
which itself contains the four moves) was something this project's own
tvision fork already had a known problem with (see the `TSubMenu +
TSubMenu` note further down this file) — `operator+` picks the
`TSubMenu&`-`TSubMenu&` overload automatically when both sides are
submenus, which chains them as two *separate* top-level menus in the
bar rather than nesting one inside the other. Worked around by casting
the inner submenu to `TMenuItem&` explicitly before the `+`, forcing
the other overload (`TSubMenu&`-`TMenuItem&`) that actually appends it
as an item within the outer menu's own list — a `TSubMenu` still
qualifies for that one too, since it inherits from `TMenuItem`, it's
just not the overload the compiler reaches for on its own. Confirmed
nested rather than sibling by checking the actual rendered menu bar:
"Queue" shows up as a single extra line inside the already-open
Torrent dropdown (with the small "►" tvision draws for anything that
opens a further submenu) rather than as a whole new top-level entry
next to Torrent and Window — and clicking it opens exactly the four
expected moves, not the wrong menu's contents.

While testing the double-click cycle specifically, ran into something
that looked at first like the click itself just not registering at
all — no RPC calls reaching the mock server, the test process simply
hanging. Traced to a real, if narrow, gap: the long-press watch that
kicks off multi-select mode (see "Multiple selection" above) didn't
check for `meDoubleClick` at all, so the second half of a double-click
— which still arrives as its own `evMouseDown` — could fall into
*that* code path and start watching for an all-new long-press on a
button that, as far as this specific event was concerned, was never
freshly pressed to begin with. Fixed by excluding an event that already
carries the flag from ever starting that watch. Once that was in place,
the actual "nothing happening" turned out to be a mistake in the test
itself, not the feature: the queue column is hidden by default (see
"Column resizing, reordering, and visibility"), and the test had only
ever hidden every *other* column rather than separately making this
one visible — so it was clicking into empty space past the only column
that was actually showing.

Verified in the same layered way as the rest of this file: the double-
click cycle end to end through a mock server tracking which of the four
`queue-move-*` RPC calls arrived and in what order (confirmed exactly
top→up→down→bottom, for the double-clicked torrent specifically); that
double-clicking any other column still opens details as before,
unaffected by any of this; that the queue menu — reached the normal
way, through a real running instance — applies to every checked torrent
when more than one is selected, not just the focused one; and the
nested-submenu structure itself both by inspecting the rendered menu
bar directly and by clicking through to its four items and reading back
which command each one actually sends.

**Usability pass on the files window and multiple selection.** Several
changes requested together: on the files window, double-click behaving
differently by which column it lands on (wanted toggles, priority
cycles), the same two actions on right-click too, and the four buttons
that used to be the only way to reach them gone entirely. On the main
list's multi-select, a shorter press-and-hold, Esc/Enter to leave
selection mode, and a "Cancel selection" entry in the context menu.

The column-aware double-click needed a new piece of `TGridView` itself:
the existing `RowActivateFn` (used elsewhere for "double-click or Enter
opens details") only ever carries a row, since it's driven by
`TListViewer`'s own generic `cmListItemSelected` broadcast — no
column info reaches it because nothing at that point in tvision's own
handling knows *where* the click landed, only what row is now focused.
Added `CellActivateFn` alongside it instead of replacing it — fired
directly from `TGridRowsView`'s own mouse handling (where the exact
click position is still available), only for a genuine double-click
(checked via `meDoubleClick`) and only outside selection mode. Reused
the header's own column-hit-test (`columnAtX()`, promoted from a
private detail of the header view to a method on `TGridView` itself so
both the header and the rows can call it) rather than duplicating that
math a second time. Confirmed safe to still let the event fall through
to `TListViewer::handleEvent()` afterward (unlike a plain single click,
which blocks synthetic tests without a live queue) by reading
`tlstview.cpp` itself: its own press-tracking loop checks for
`meDoubleClick` *before* ever calling `mouseEvent()`, so an event that
already carries the flag on arrival exits that loop immediately rather
than blocking on it.

The right-click context menu on the files window is new — this window
never had one before — built the same way `TorrentListWindow`'s own
already does, and deliberately checked against the exact mistake fixed
there in the previous round (handling the right-click check *before*
calling `TListViewer::handleEvent()`, not after) rather than trusting
"looks the same as the working one" on sight.

The long-press threshold (`kLongPressMs`) went from 3000 to 800 —
functionally a constant-value change, but it demonstrated something
about testing time-based UI that's worth a note either way: 3 seconds
had already been verified once with genuine elapsed time on a live
terminal, and this change re-ran that same verification rather than
assuming a simple number swap couldn't have broken the surrounding
logic (the elapsed-time comparison, the drain-the-eventual-release loop
afterward) on its own.

"Cancel selection" only appears in the context menu while a selection
is active, built by conditionally appending one more `TMenuItem` to the
chain already being built — worth noting because `operator+` for
`TMenuItem` (see tvision's own `menu.cpp`) returns a reference to the
*first* item in the chain, not the one just added, having walked to the
tail and linked the new item on there; the natural-looking `items =
items + newItem` assigns *through* that reference instead of rebinding
it (since `items` is declared as a reference, not a pointer), which
would silently corrupt the first item's own fields rather than extend
the chain. The mutation already happens as a side effect of the `+`
call itself, so the fix was to drop the assignment entirely, not to
find a different way to reassign.

Verified in the same layered way as the last several changes here:
the column-routing itself in isolation (a double-click at a specific
content-x lands on the column actually under it, checked at two
different x positions in the same grid); the wanted toggle and the
priority cycle end to end through a mock server with real mutable
state, driven by the exact same double-click mechanism a real click
would use — including catching a mistake in the test itself along the
way (a wrong x coordinate landing on a separator's hit-test range
rather than the intended column, since a column's clickable span
includes its trailing separator — the toggle test's initial "nothing
changed" result was that, not a bug in the toggle); the files window's
own context menu confirmed to fire without blocking, same test shape as
the main list's; Esc and Enter each confirmed to actually flip
`isInSelectionMode()` off; the reduced long-press threshold confirmed
against real elapsed time on both sides (entering at just past 800ms,
not entering at 300ms); and "Cancel selection" confirmed present in a
real rendered context menu, opened for real while a selection was
active.

**Right-click never opened the context menu on the torrent list — on
any terminal, not a PuTTY-specific issue as first suspected.** Asked
directly whether this was really a client-side setting, backed by two
pieces of concrete counter-evidence: right-click worked fine elsewhere
(clicking menus), and tvision's own `tvedit` sample opens a context
menu on right-click with the same terminal — both pointed at this
app's own code rather than the terminal or PuTTY's mouse-button
configuration. The actual bug: the right-click check ran *after*
calling `TListViewer::handleEvent()` (the base class), but that base
method's own click-tracking never checks which button was pressed at
all — it unconditionally enters its own internal loop for any
`evMouseDown`, blocking until the button is released and overwriting
`event.what` to `evMouseUp` in the process. Checking "was this
`evMouseDown` with the right button" afterward could never succeed,
regardless of which button was actually pressed or which terminal sent
it — by the time control returned there, `event.what` was never still
`evMouseDown`. Fixed by moving the right-click handling to run before
the base class ever sees the event, the same way the left-click
handling added for multiple selection already had to.

Worth being honest about here: an earlier reply confidently reported
verifying this worked, based on a synthetic test blocking for close to
the expected duration and the desktop's window count increasing while
it did. That evidence was real, but didn't actually distinguish the
context menu genuinely opening from the *old, buggy* code's own
internal block inside `TListViewer::handleEvent()`'s click-tracking
loop happening to look the same from outside — both block for a
similar-looking duration when nothing arrives to end them. The fix
this time is checked with a test that can't be confused that way: a
grid built without going through the wider application at all, with a
callback that does nothing but record that it fired and which row it
got — completing instantly, with no blocking involved, which is only
possible if the base class's own loop was never entered in the first
place. Confirmed it fires for the exact row clicked, and confirmed
left-click and left-click-during-selection still behave exactly as
before after reordering the checks around it.

**The files window had the same missing-row-highlight bug already fixed
elsewhere, still showed "Yes"/"No"/"Mixed" text for wanted instead of a
checkbox, and had no double-click shortcut for toggling it.** Reported
together, but turned out to be a different window than the one just
worked on (the main list's own multi-select checkboxes already showed
`[X]`/`[ ]` correctly) — the files window (`TorrentFilesWindow`) had
simply never gotten any of these three passes. Fixed the same way each
was fixed elsewhere: an explicit `setRowColorCallback()` (black-on-white
when focused, the same fixed look used everywhere else in this app);
the wanted column now returns `[X]`/`[ ]`/`[-]` instead of translated
Yes/No/Mixed text — a tri-state checkbox convention for the case where
a folder's descendants disagree, rather than trying to squeeze that
into a two-state glyph (the now-unused `Str::ValueYes`/`ValueNo`
translations were removed, though `ValueMixed` stays — the priority
column's own "Mixed" case still needs it); and `setRowActivateCallback()`
now calls the same `toggleWantedForFocused()` the button already used,
so a double-click does what the button does without needing to reach
for it.

While verifying the files window's double-click, checked the main
list's own multi-select double-click too and found a real bug there:
clicking a row toggles it, but the *second* mouseDown of a double-click
also arrives as its own `evMouseDown` — un-checked by the same code,
toggling it right back off — and, since nothing stopped the event from
also reaching `TListViewer`'s own double-click detection afterward,
opened a details window mid-selection on top of that. Fixed by
recognizing the second click (`meDoubleClick` in `event.mouse.
eventFlags`) and skipping the toggle for it, then consuming the event
outright — never letting it reach the base class's own activate logic
at all while in selection mode, rather than only suppressing part of
what it would otherwise do.

Verified each of the four fixes concretely rather than by inspection
alone: the row-focus color and the checkbox glyphs off real rendered
terminal output; the double-click's actual effect by sending the exact
command the button and the row-activate callback both funnel into and
then re-querying a stateful mock server afterward, confirming the file
that started "not wanted" really did flip to wanted rather than just
trusting that the command fired; and the main list's double-click fix
by simulating a first click (checked, confirmed) followed by a second
with `meDoubleClick` set, confirming the checked state doesn't change
back *and* that no details window was requested. Two dead ends along
the way, both artifacts of the test harness rather than the
application: the same non-recursive-search mistake already made and
caught earlier this session (`forEach()` only visits direct children,
so a `TListViewer` nested one level inside the grid needs the search to
recurse into any `TGroup` it finds, not just scan the window's own
immediate children), and a mouse-click test that hung because the test
itself never called `refresh()` after `setRowCount()` — meaning the
row-count `range` a synthetic click gets checked against was still 0,
routing the click into a fallback path that (correctly, if that path
were ever really reached with no live event queue behind it) blocks
waiting for input that a synthetic test has no way to supply.

**"Files" had no entry in the Torrent menu bar**, only in the right-
click context menu — `cmShowFiles` was already fully wired up (the
command handler, the keyboard-accessible batch-selection path just
fixed above), but no `TMenuItem` for it had ever been added to the
menu bar's own Torrent submenu, so it simply wasn't reachable from
there. Added right after "Details", matching where it already sits in
the context menu. Verified by actually opening the real menu on a live
running instance and checking "Files" shows up in it, not just that
the command itself still works when triggered some other way.

**"Details" and "Files" were left out of multiple selection** — every
other Torrent-menu action already read its targets from
`targetTorrents()` (see the multi-select entry below), but these two
still called `selectedTorrent()` directly, so checking several torrents
and opening either one only ever affected the focused row, same as
before multi-select existed at all. Fixed by rewriting both the same
way: loop over `targetTorrents()`, keeping each one's own existing
"reuse an already-open window for this torrent id instead of
duplicating it" check per torrent rather than just once — a user
picking Details with three torrents checked, one of which already has
its details window open, should get two new windows and the existing
one brought to front, not one of those three silently skipped or
duplicated. Both exit selection mode afterward, matching every other
action. Verified with a mock server exactly the way the batch actions
were: checking two specific torrents and firing the real Details
command opens exactly two details windows — confirmed by counting them
on the desktop before and after, not just assuming — and the same for
Files; selection mode exits on its own in both cases. Caught a
mismatched mock server field check while narrowing this down (checking
for a field name to distinguish an on-demand torrent-details fetch from
the periodic full-list refresh, when the full list already requests
that same field for its own optional "Location" column) that made an
earlier test run look like the real bug was still there — worth noting
since it's the kind of thing that would produce a false "still broken"
result if not caught, distinguishing the two request types by whether
`ids` is present instead resolved it.

**Added multiple selection**, in three layers, each verified before
moving to the next rather than all at once. First, in `TGridView`
itself (see `src/tgridview/README.md` for the design notes — opt-in via
a new `gvMultiSelect` option, so every other grid in this app keeps
behaving exactly as before): a leftmost `[X]`/`[ ]` checkbox column,
shown only once `enterSelectionMode()` is called, fixed in place and
never affected by horizontal scrolling the way a regular column would
be. Getting there via a 3-second press-and-hold turned out to need a
different approach than the drag-tracking already used elsewhere in
this widget (see `dragResize()`): `TView::mouseEvent()` only returns
once an event matching its mask actually occurs, which would leave a
"holding still, not moving" gesture blocked indefinitely waiting for a
move event that never comes. Calling `getEvent()` directly instead
relies on `TProgram::getEvent()`'s own wait timeout — it still returns
periodically with `evNothing` even with nothing happening at all (the
same path `idle()` runs on) — which is what lets an elapsed-time check
actually get a chance to run.

Second, wiring it into `TorrentListWindow`: every existing Torrent-menu
action (Start, Stop, Remove, Delete with files, Start Now, Verify,
Reannounce) already funneled through its own dedicated `*Selected()`
method, so adding a shared `targetTorrents()` — every checked row if
selection mode is active and at least one is checked, the existing
single focused-row behavior otherwise — meant each of those seven
methods only needed a loop added, not separate logic apiece. Third, the
periodic list refresh (`App::idle()`) now skips itself entirely while
selection mode is active: it re-fetches and re-applies the current
sort/filter, which can reorder the underlying list, and a checked row
is tracked by index — reordering while rows are checked would silently
apply an action to whatever torrent now sits at that index instead of
the one actually checked. Holding the list still until the user
finishes (or cancels) avoids that outright rather than trying to remap
indices across a refresh.

Verified in layers matching the implementation: the widget's own
mechanics (enter/exit, toggling, the checkbox staying fixed through
horizontal scrolling, the 3-second gesture specifically distinguishing
a long hold from a short click — captured off a live terminal with real
elapsed time, not simulated) directly, before touching the application
at all; then, with a mock server, that checking two specific torrents
and firing Start from the real menu command sends the RPC call for
*both* of their actual IDs, not just the focused one, and that
selection mode exits on its own once the action runs; and that holding
the app in selecting state across several seconds of repeated `idle()`
ticks leaves the checked row exactly where it was, confirming the
refresh pause actually holds rather than just usually not mattering.

**The previous fix for the scrollbar's own visibility being reset by
tvision's insertion machinery (see the entry just below) introduced a
worse bug of its own: repeatedly toggling columns visible/hidden could
grow the grid's rows area without bound, eventually overlapping the
application's own status line** — reported as the status bar being
replaced by a torrent's name, restored only by resizing the window
(which forces a fresh layout pass). Root cause: that fix's correction
ran on every single `draw()` call — necessary, since the previous bug
was tvision re-flipping the scrollbar's visibility independently of
this widget's own calls — but it decided WHETHER to resize by
re-reading that same externally-flippable `sfVisible` state. If
something keeps flipping it back between draws, every single draw sees
a "mismatch" again and grows (or shrinks) the rows area by another row,
every time — not just once per actual transition. Fixed by never
reading that state for the resize decision at all: a new, private
`hScrollBarRowReserved_` flag, updated only by this widget's own code
and never touched by anything external, is now the sole authority for
whether a resize happens. The scrollbar's own `show()`/`hide()` call is
still repeated unconditionally on every `draw()` (so external
interference with its on-screen appearance keeps getting corrected),
but that call no longer feeds back into whether the row gets
resized — decoupling "does this look right" from "do we resize",
which is what let the two keep re-triggering each other before.
Verified far more rigorously than the visibility fix alone required,
given the severity: 50 consecutive redraws with no column changes
leave the row count exactly unchanged (not just "still looks about
right"); toggling a column visible and hidden 20 times in a row lands
on the exact same two row-count values every single time, with no
drift in either direction; and, in the real application with real
`refresh()` calls (not a synthetic single check), 30 rapid visibility
toggles leave the window's own border and the application's status
line exactly where they belong, with the horizontal scrollbar itself
still fully contained inside the window rather than having crept
outside it.

**Adding or removing columns through "Manage columns..." didn't
reliably recalculate the horizontal scrollbar.** Two real, separate
issues found along the way, though a full end-to-end keyboard-driven
reproduction inside the dialog itself wasn't achievable in this
environment (see the note below) — verified as thoroughly as that
allowed instead:
- The dialog's initial focus used `selectNext(False)`, which is a
  no-op unless something is already the dialog's current view. Left
  initial keyboard focus wherever `insert()` happened to leave it —
  not necessarily the meta-grid at all — meaning arrow keys and Enter
  right after opening the dialog could land on nothing useful until
  the user clicked something themselves first. Fixed with an explicit
  `metaGrid->select()` instead, so keyboard-first interaction has a
  predictable starting point regardless of insertion order.
- `App::showColumnManagerDialog()` didn't force a redraw of the target
  grid once the dialog closed. The underlying recalculation itself
  turned out to be correct and synchronous — confirmed directly, calling
  `TGridView::setColumnVisible()` the exact same way the dialog's own
  "Toggle visible" does, in both directions (hiding columns until
  content fits, and revealing columns until it doesn't) — but nothing
  guaranteed the now-revealed window actually got told to repaint the
  instant the covering dialog was gone, rather than at its next
  unrelated redraw. Added an explicit `grid->refresh()` right after the
  dialog closes as a belt-and-suspenders alongside `TGridView`'s own
  draw()-time self-correction (see the entry above) — the same
  "whatever the state should be, re-validate it before it would ever
  reach the screen" principle, applied at the point most likely to
  matter for this specific interaction.

Reproducing the reported symptom itself — toggling a column's
visibility *through the dialog's own keyboard navigation* and observing
whether the scrollbar updated — wasn't achievable through this
environment's terminal automation; every attempt to drive the dialog
with simulated keystrokes either failed to actually move focus onto a
row (confirmed separately: the state was provably unchanged afterward,
not just unconfirmed) or didn't reliably close the dialog afterward.
Rather than claim a full reproduction that didn't actually happen,
verification here rests on the two fixes above, each confirmed directly
and independently — the recalculation logic itself, and this window's
own redraw once revealed — covering what's realistically checkable
without that missing piece.

**The horizontal scrollbar's hide-when-not-needed (see the entry just
below) worked right after construction but not once the window was
actually inserted onto the desktop** — reported with two screenshots
after the previous fix, showing it still visible with several columns
turned on. First checked whether it was actually a bug at all: the
specific columns shown do add up to more width than a common terminal
size, so the scrollbar being visible there was correct — but the same
setup, reproduced directly with a wider terminal where the columns
genuinely all fit, showed the scrollbar staying visible anyway, range
correctly computed as `[0, 0]` (nothing to scroll) but never actually
hidden. Traced to `relayout()`'s hide/show logic running correctly
during construction — confirmed directly by reading the scrollbar's own
`sfVisible` state right after building the window, off — but something
in tvision's own view-insertion internals (`TGroup::insertBefore()`'s
exposure cascade is the most likely path, though pinning down the exact
mechanism further wasn't worth it once a solid fix existed) flips it
back on independently of this widget's own `hide()`/`show()` calls,
confirmed by reading that same state again immediately after
`deskTop->insert()` — with no `relayout()` call happening in between.
Fixed defensively rather than by chasing the exact tvision internals
further: the header's own `draw()` now re-validates and corrects the
scrollbar's visibility every time it actually draws, not only when
`relayout()` runs — so whatever tvision's insertion machinery did gets
silently corrected before it would ever reach the screen, regardless of
the precise reason it happened. Verified directly, in the real running
application rather than a synthetic reproduction: a 100-column terminal
(too narrow for the specific columns reported) shows the scrollbar, a
150-column and a 160-column one (both wide enough) don't — with no
special handling needed to trigger the correction, exercised purely by
the application's own normal draw cycle.

**The horizontal scrollbar (see the entry just below for how it was
added) stayed visible even when every column already fit** — asked
directly whether it made sense to hide it rather than always reserving
a row for a control with nothing to do. It does; `TGridView::relayout()`
now hides it (and hands its row back to the rows/vertical-scrollbar
area, which grow to fill it) whenever the content no longer exceeds the
view, and shows it again (shrinking rows/vertical-scrollbar back down
by one) the moment it would. Both directions run through the same
check on every `relayout()` — every column add/remove/resize/
reorder/show-hide already calls it — rather than needing a caller to
remember to ask. Verified two ways: directly, by reading the
scrollbar's own visibility state and range before and after a column
resize that pushes content from fitting to not fitting (and the
reverse), confirming both the show/hide and the row reclaimed/
surrendered actually happen, not just that content becomes clipped or
not; and in the real application itself — an 100-column terminal (too
narrow for the default 7 columns' combined width) shows the scrollbar,
a 160-column one (wide enough) doesn't, with one more row of torrents
visible where it would have sat.

**The column manager's "Visible" marker showed "Yes"/"No" text and had
no double-click shortcut**, unlike the files window's own similar
"wanted" toggle. Both fixed: the marker is now `[X]`/`[ ]` — a checkbox
glyph doesn't need translating the way the rest of this app's text
does, so `TGridColumnManagerLabels`' own defaults changed rather than
adding new translated strings for it — and double-clicking a row (or
pressing Enter on it) triggers the same toggle the "Toggle visible"
button already did, reusing its exact logic rather than a second copy
of it.

**Added horizontal scrolling to `TGridView` itself** — previously,
columns past whatever fit the view simply didn't appear at all, with no
way to reach them; asked directly after enough optional columns had
been turned on in the main list to run past a normal terminal's width.
A second `TScrollBar`, laid out along the bottom the same way the
existing one runs down the right edge, drives a shared horizontal
offset that both the header and the rows read from when drawing —
matters because they have to redraw in *sync*: reusing
`TListViewer`'s own built-in reaction to its horizontal scrollbar
changing handles that automatically for the rows, but the header isn't
a `TListViewer`, so `TGridView`'s own `handleEvent()` catches the same
broadcast for it too, or the two would drift out of alignment the
moment either redrew independently. The genuinely fiddly part was the
left edge: `TDrawBuffer::moveStr()`'s indent parameter is a `ushort`,
which can't represent a negative position, so a column partially
scrolled past the left edge can't just be drawn at a negative x the way
the right edge already safely clips on its own (the buffer's own fixed
size already handles overflow there, which is exactly why extra
columns used to just silently vanish instead of corrupting anything).
Solved by clipping the *string* instead of the position — a small
codepoint-aware `skipLeadingUtf8()` (the mirror image of the
`truncateUtf8()` already used for the right edge) drops however many
leading display columns have scrolled past, then the shortened text is
drawn starting at indent 0. Every existing hit-testing helper
(`columnAtX()`, the sort glyph, the resize separator, the reorder
markers) needed no internal changes at all — they already worked in
content-relative coordinates; only the one conversion from screen-
relative mouse position to content-relative, done once at the top of
the header's `handleEvent()`, needed adding. Verified directly: the
computed scroll range is exactly zero when everything already fits (no
behavior change for the overwhelmingly common case); it's the exact
expected non-zero value once content exceeds the viewport; a header
label straddling the scroll boundary clips correctly at a multi-byte
UTF-8 character (a plain byte-based clip would have corrupted it) with
no crash; and — the check that actually matters, not just that
something *looks* scrolled — after scrolling, a click at the position
where a column's sort glyph now sits lands on that *column specifically*
(verified by which column the sort callback fires for), not
whatever used to be there before the offset was accounted for.

**Neither the tracker list's own rows nor the column manager's meta-grid
were visibly highlighting the focused row.** Same root cause each time
this has come up before (the main torrent list, then the column
manager's own meta-grid the first time it existed): without an explicit
row-color callback, `TGridView` falls back to `TListViewer`'s inherited
palette colors, which don't contrast enough inside a `TDialog` to
actually notice which row is focused — even though the underlying focus
tracking itself was working correctly the whole time. Fixed both the
same way as before: an explicit `setRowColorCallback()` returning a
fixed black-on-white for the focused row regardless of the host
dialog's own palette. Verified this time by reading the actual rendered
color attributes off a live terminal screen (a real pty, foreground/
background per cell), not just the text content: the focused row comes
back black-on-white, every other row white-on-blue, genuinely distinct
— rather than trusting that matching the same code pattern used
elsewhere was enough on its own.

**The tracker list's column layout wasn't saved anywhere** — every time
you opened a tracker window it started from scratch, unlike the main
list's own columns. Fixed the same way the main list's own layout is:
three new `AppSettings` fields (`trackerColumnWidths`/`Order`/
`Visible`), loaded/saved in `Config.cpp` the same way, and threaded as
constructor parameters all the way from `App` down through
`TorrentListWindow` and `TorrentDetailsWindow` to wherever a tracker
window actually gets created — the same explicit, one-piece-at-a-time
parameter passing this project already uses throughout, rather than
handing the whole `AppSettings` down the chain. `App::
showColumnManagerDialog()`'s existing "was the grid just edited the
main list's own?" check gained a second branch: if the focused window
is a `TrackerListWindow` instead, its layout is persisted to the
tracker-specific fields, not the main list's. Unlike the main list
(exactly one, so identity comparison is enough), several tracker
windows can be open simultaneously — they're designed to share a single
saved layout rather than one per torrent, so `App::shutDown()`'s own
backstop (for direct header drags/reorders that never went through
"Manage columns...") just persists whichever tracker window it finds
first, rather than needing to reconcile several against each other.
Verified directly: the settings file round-trips all three tracker
fields correctly, independently of the main list's own (and separately
from it, confirmed by checking both sets of values, not just one);
resizing and hiding a tracker column and then closing "Manage
columns..." while that window has focus writes the exact values
changed into `settings.json`, not just *some* update; and — the part
most likely to have been wrong — opening a **new** tracker window
afterward, for a **different** torrent than the one that was just
edited, actually starts from that saved layout rather than this
window's own hardcoded defaults.

**"Manage columns..." became a single, focus-aware menu entry** instead
of one per `TGridView`-based window — asked directly, after the tracker
list got its own "Columns..." button (see the migration entry just
below): wasn't a separate entry per window exactly the kind of thing
that had already been rationalized once before, for the main list's
own resize/reorder/visibility? Added `App::focusedGrid()`: walks
`deskTop->current` (whichever window has focus) for a `TGridView` among
its own children — which covers every shape this app builds one in,
`TorrentListWindow`'s `TGridWindow` base as much as a plain `TDialog`
with one embedded directly, like the tracker list — without needing to
know which specific window class it's looking at. `App::idle()` (which
already ran every event-loop tick for the periodic refresh) enables or
disables the menu command based on what that returns, so the item is
visibly greyed out — not just a silent no-op once clicked — the moment
focus moves to a window with no grid in it at all, or a modal dialog.
The tracker list's own now-redundant "Columns..." button was removed
in the same change, along with its dedicated command and translated
label. Only the main list's own column layout is ever persisted to
`settings.json`; a different grid's changes apply live but aren't
saved, checked by comparing the acted-on grid against the main list's
own by identity rather than by which window it came from. Verified
directly, inside an actual running application rather than only
checked for compiling: the command is enabled while the main list has
focus, becomes disabled the moment a window with no grid (torrent
details) takes focus, and re-enables once the tracker list — a
different grid entirely — takes focus in turn; confirmed by reading
tvision's own `commandEnabled()` at each step, not inferred from
whether a click happened to do something.

**Migrated the tracker list window to `TGridView`** too, the same way
the main torrent list and the files window already had — asked directly
whether it made sense, given `TGridColumnManagerDialog` (see the
generalization entry below) already existed and had nothing torrent-
specific left in it. Replaced the hand-rolled `TrackerListViewer` (a
custom `TListViewer` with its own `getText()` string-building and a
separate `TStaticText` for the header) with a plain `TGridView`
carrying 6 columns. Originally reachable through a "Columns..." button
of its own, since folded into the single focus-aware menu entry above —
the button existed only briefly, replaced once the same need showed up
in a second window and made the pattern worth generalizing rather than
repeating. One deliberate choice carried over unchanged from the
original design: every column has `sortable = false`, since
Transmission already returns trackers in tier order and that's the
order that matters here — resizing/reordering/hiding the columns
themselves is still offered, since that's independent of row order.
Verified against a mock server: the rendered rows match the underlying
data for every column, including the `-1` → "N/A" case Transmission
uses for counts it doesn't have yet; every column actually has
`sortable = false` while still being resizable and movable; and
double-clicking a row still opens the tracker detail window through the
new chain, confirmed by watching the desktop's window count go up by
exactly one.

**"Manage columns..." moved into `tgridview/`, generalized to operate on
any `TGridView` instead of specifically `TorrentListWindow`.** Asked
directly whether it made sense as a reusable piece — analyzed first
(before writing anything) how much of it was actually torrent-list-
specific: almost none, it turned out. `TorrentListWindow::
startColumnResize()`/`startColumnReorder()` were already one-line
forwards to `TGridView::startKeyboardResize()`/`startKeyboardReorder()`;
"Toggle visible" was already just `setColumnVisible()`; and the meta-
grid's own "Column" label column was reading from a locally-duplicated
`kColumnLabels[]` array instead of `column(i).header` directly, which
turned out to be unnecessary rather than something reset actually
needed. The one genuine gap was "Reset": nothing on `TGridView` knew
what a column's *default* width/visibility had been once it was
changed, because nothing had ever needed to remember that before.
Added a small, generic fix rather than an app-specific one: `TGridView`
now keeps a `defaultColumns_` snapshot alongside `columns_`, taken at
the exact moment each column is `addColumn()`-ed, and a new
`resetColumns()` restores width/visibility from it (plus resetting
order to identity) — independent of anything the app-level `Torrent
ListWindow::setupColumns()` used to do by tearing down and rebuilding
every column from scratch. The other real design question was text:
this module has no dependency on any particular app's translation
system by design, so the dialog's title/column headers/button labels
are a small `TGridColumnManagerLabels` struct with plain-English
defaults, overridable by the caller — this app's own `App.cpp` builds
one from its existing `tr()`-translated strings right before calling
`createColumnManagerDialog()`, so the generic version stays genuinely
app-agnostic while this app's own copy is still fully translated.
Verified directly: `resetColumns()` restores the *original* defaults
even after every property (width, visibility, order) has been changed,
not some intermediate state; the dialog works standalone against a
plain `TGridView` with no `TorrentListWindow` involved at all, correctly
reading column labels live from the real columns rather than a
duplicated array; custom labels passed in are actually used, and
omitting them falls back to the English defaults without crashing; and
the Reset button, exercised through the dialog's own command dispatch
(not called directly), produces the same restored values on the real
grid.

**The files window originally showed every file as one flat row**,
full relative path and all — asked directly whether folders were
handled specially, and they weren't: a torrent with subfolders (a TV
season, say) would show every file with its whole
`Season1/Episode01.mkv`-style path crammed into the name column, with
no way to act on a whole folder at once. Fixed by actually building a
tree from the files' own paths (split on `/`, which is what
Transmission's RPC always uses regardless of the daemon's own host OS)
rather than treating `files_` (the flat, RPC-order list `torrent-set`
addresses files by index in) as the display order directly. A separate
`rows_` list — one row per real file *or* per synthetic folder,
indented by depth, each carrying the real file indices it represents
(one for a file, every descendant's for a folder, collected recursively
so a sub-subfolder's files still count toward every ancestor folder
above it) — is what the grid actually renders and what every action
reads from, leaving `files_` itself untouched as the thing indices are
still resolved against. A torrent with no folders at all produces
exactly the same flat one-row-per-file list this had before, which is
what most of the testing before this point had already exercised
without anyone noticing the gap. Verified directly, with a torrent
built specifically to have a nested folder (`Season1/Extras/`, two
levels deep) and files in different states: a folder row's size/percent
are genuinely summed across every descendant, not just its direct
children; "wanted" and "priority" show "Mixed" only when descendants
actually disagree, not for a folder with just one file under it;
toggling a folder in a mixed wanted-state turns every descendant *on*
(never leaves it ambiguous which way "toggle" should go when they
already disagree); and — the part most likely to have a bug — the exact
file indices sent to `torrent-set` for a folder action are the real,
original RPC indices of every file beneath it, however deep, not
positions within the display list.

**Added per-file selection and priority** (`TorrentFilesWindow`,
reachable from "Files..." in the torrent list's right-click menu),
requested with an example screenshot from another Transmission client's
own files view. Built on `TGridView` for the list itself — another data
point for the widget holding up outside the main torrent list it was
generalized from. Required two new `torrent-get` fields Transmission
returns as parallel arrays rather than one combined one (`files` for
name/size, `fileStats` for wanted/priority/completed bytes, both in the
same per-file order) — `TransmissionClient::getTorrentFiles()` zips them
together by index into one `TorrentFile` per file, and the "Toggle
wanted"/priority buttons/"Select all"/"Select none" actions all address
files the same way, by that index, via `torrent-set`'s `files-wanted`/
`files-unwanted`/`priority-low`/`priority-normal`/`priority-high`.
Verified directly against a mock server: the merge produces the correct
combined data per file (not just each array read independently); each
action sends exactly the RPC call it should — the right field name, the
right file index (or *every* index, for Select all/none) — not simply
that some request went out; and the grid's rendered cells match the
underlying data for every column, including a partially-downloaded
file's percentage.

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
