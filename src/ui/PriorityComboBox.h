#pragma once

#include "../tvision-ext/TComboBox.h"

// A combo box for picking a torrent's bandwidth priority (Low/Normal/
// High) — the single place to change a torrent's priority now that
// this project no longer has a "Priority" menu (menu bar or context
// menu): see TorrentDetailsWindow.h/.cpp, where this is inserted, and
// the double-click-to-cycle behavior on the main grid's own Priority
// column (TorrentListWindow::cyclePriorityForRow), which is kept
// alongside this as a second, faster way to do the same thing.
//
// Built on this project's own vendored TComboBox, same as
// LanguageComboBox — see src/tvision-ext/TComboBox.h for why tvision
// itself has no combo/dropdown widget of its own.
class PriorityComboBox : public TComboBox {
public:
    // `initial` matches Transmission's own bandwidthPriority encoding:
    // -1 = low, 0 = normal, 1 = high (see Torrent::bandwidthPriority in
    // Torrent.h) — the same three values TorrentListWindow::
    // setPriorityForSelected()/cyclePriorityForRow() already use, so
    // priority() below can be passed straight to
    // TransmissionClient::setPriority() with no translation.
    PriorityComboBox(const TRect& bounds, int initial);

    // TComboBox::value is the ulong the matching TComboItem was built
    // with; the constructor below sets it to (ulong)(int) for each of
    // the three priority values, so the reinterpret back to int here is
    // exact, not just "hopefully in range" — value can only ever be one
    // of the three this combo box was given. ulong -> int would truncate
    // on some platforms for values that don't fit, but -1/0/1 always do.
    int priority() const { return static_cast<int>(static_cast<long>(value)); }
};
