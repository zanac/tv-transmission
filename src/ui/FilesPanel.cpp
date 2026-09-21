#include "FilesPanel.h"
#include "Strings.h"
#include "TextUtil.h"

#define Uses_TView
#include <tvision/tv.h>

namespace {

// Same fix, same reason as StatusPanel.cpp's own TPanelBackground —
// see its own comment for the full story (a container-level draw()
// override only ever got asked to repaint the rows something else
// already invalidated, never the panel's own full extent on its own).
class TPanelBackground : public TView {
public:
    TPanelBackground(const TRect& bounds, TColorAttr color)
        : TView(bounds), color_(color) {
        growMode = gfGrowHiX | gfGrowHiY;
    }
    void draw() override {
        TDrawBuffer b;
        b.moveChar(0, ' ', color_, size.x);
        for (int y = 0; y < size.y; y++) writeLine(0, y, size.x, 1, b);
    }
private:
    TColorAttr color_;
};

} // namespace

FilesPanel::FilesPanel(const TRect& bounds, TransmissionClient& client)
    : TGroup(bounds), client_(client) {
    growMode = gfGrowHiY | gfGrowHiX; // full height AND width of whatever
                                       // TorrentListWindow gives it — this
                                       // panel's own bounds change on every
                                       // relayout anyway (see TorrentListWindow.cpp),
                                       // but matching StatusPanel's own
                                       // growMode here too keeps both panels
                                       // consistent about it.

    // 0x30 (the same cyan the Status panel's own checkboxes use — asked
    // for directly, after gray/black wasn't wanted after all).
    insert(new TPanelBackground(TRect(0, 0, size.x, size.y), TColorAttr(0x30)));

    // (0, 0, size.x, size.y), not (1, 1, size.x-1, size.y-1) — that
    // margin was copied from TGridWindow's own grid setup without
    // reconsidering that TGridWindow's own version needs it to clear
    // ITS OWN frame (a real TWindow border), which this panel doesn't
    // have at all: nothing draws a border around this panel itself
    // (the divider TorrentListWindow draws sits OUTSIDE this panel's
    // own bounds entirely — see TorrentListWindow.cpp's own
    // relayoutPanels()). The extra margin left an empty row above the
    // header and another below the last row, reported directly as
    // "content starts one row down, skipping a space between header
    // and border" — there was no border here to skip past.
    // gvResizableColumns — missing until now. Without it, "Name" was
    // stuck at a fixed width (nameCol.width below) with no way to see
    // more of a filename longer than that, the same way the main
    // torrent list's own "Name" column would be if it lacked this same
    // option (see TorrentListWindow::setupColumns()) — asked about
    // directly ("shouldn't there be horizontal scroll for long
    // filenames?"): there IS a horizontal scrollbar built into
    // TGridView already, but it scrolls between whichever columns
    // don't all fit, not within one column's own truncated text — the
    // real fix is letting the user drag "Name" wider (dragging the
    // header border, same gesture as the main list), which is exactly
    // what reveals more of it, past that width, via the same
    // horizontal scroll if "Name" and "Wanted" together end up wider
    // than this panel's own viewport.
    grid_ = new TGridView(TRect(0, 0, size.x, size.y), gvResizableColumns);
    // Missing until now — grid_ never had its own growMode set at all,
    // meaning it stayed fixed at whatever size it happened to be
    // constructed with (this panel's own placeholder size, before ever
    // being resized to its real one) and never grew when this panel
    // later did. TGridView's own new changeBounds() override (see its
    // own doc comment in TGridView.h) exists specifically to correctly
    // re-lay-out grid_'s own children — including its scrollbar —
    // whenever grid_ ITSELF gets resized, but that override can only
    // ever run if something actually resizes grid_ in the first
    // place, which nothing did without this.
    grid_->growMode = gfGrowHiX | gfGrowHiY;
    TGridColumn nameCol;
    nameCol.header = tr(Str::HeaderFileName);
    nameCol.width = size.x - 8;
    nameCol.minWidth = 8;
    grid_->addColumn(nameCol);
    TGridColumn wantedCol;
    wantedCol.header = tr(Str::HeaderFileWanted);
    wantedCol.width = 5;
    wantedCol.minWidth = 5;
    wantedCol.resizable = false; // fixed width for the "[X]"/"[ ]"/"[-]"
                                  // glyph — nothing to gain by resizing it,
                                  // same reasoning as the main list's own
                                  // "Done" column (progress bar, also fixed
                                  // width) — see TorrentListWindow::setupColumns().
    grid_->addColumn(wantedCol);

    grid_->setCellTextCallback([this](int row, int col) -> std::string {
        if (row < 0 || row >= (int)rows_.size()) return "";
        const FileTreeRow& fr = rows_[row];
        if (col == 0) {
            std::string indent(fr.depth * 2, ' ');
            return indent + fr.name + (fr.isFolder ? "/" : "");
        }
        // col == 1: same tri-state convention as TorrentFilesWindow's
        // own wanted column ("[-]" for a folder whose own descendants
        // disagree) — see that window's own comment on why, reused
        // here verbatim rather than re-derived.
        bool allWanted = true, noneWanted = true;
        for (int idx : fr.fileIndices) {
            if (files_[idx].wanted) noneWanted = false; else allWanted = false;
        }
        if (allWanted) return "[X]";
        if (noneWanted) return "[ ]";
        return "[-]";
    });
    // 0x30 throughout — asked for directly ("il verdino usato nei
    // check box"), over the gray/black uniform look a previous request
    // asked for instead: same cyan the Status panel's own checkboxes
    // use, both normal and focused rows, plus the header.
    grid_->setRowColorCallback([](int, bool) -> TColorAttr {
        return TColorAttr(0x30);
    });
    grid_->setHeaderColorCallback([]() -> TColorAttr { return TColorAttr(0x30); });
    grid_->setCellActivateCallback([this](int, int col) -> bool {
        if (col == 1) toggleWantedForFocused();
        return true;
    });
    insert(grid_);
}

void FilesPanel::handleEvent(TEvent& event) {
    TGroup::handleEvent(event);
}

TColorAttr FilesPanel::mapColor(uchar color) {
    // grid_'s own cells/header don't go through this at all (see the
    // constructor's own comment — TGridView's colors are fixed
    // defaults, reached via setRowColorCallback()/
    // setHeaderColorCallback() instead), but its own vertical scrollbar
    // is a plain TScrollBar — a standard widget that DOES resolve
    // through the normal palette chain, and without this override it
    // was landing on whatever this panel's own OWNER (TorrentListWindow,
    // a TWindow) gives an unstyled scrollbar, not matching the rest of
    // this panel at all — reported directly ("the scrollbar didn't
    // look right"). Matches this panel's own 0x30 (see the
    // constructor's own comment) for visual consistency, now that the
    // rest of the panel no longer uses the computed real-dialog gray
    // this originally matched.
    (void)color;
    return TColorAttr(0x30);
}

void FilesPanel::showTorrent(int torrentId, const std::string& torrentName) {
    torrentId_ = torrentId;
    refresh();
}

void FilesPanel::refresh() {
    if (torrentId_ < 0) {
        files_.clear();
        rows_.clear();
    } else {
        files_ = client_.getTorrentFiles(torrentId_);
        rows_ = buildFileTreeRows(files_);
    }
    if (grid_) {
        grid_->setRowCount((int)rows_.size());
        grid_->refresh();
    }
}

void FilesPanel::toggleWantedForFocused() {
    if (!grid_ || torrentId_ < 0) return;
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)rows_.size()) return;
    const std::vector<int>& indices = rows_[row].fileIndices;
    if (indices.empty()) return;
    bool allWanted = true;
    for (int idx : indices) if (!files_[idx].wanted) { allWanted = false; break; }
    client_.setFilesWanted(torrentId_, indices, !allWanted);
    refresh();
}
