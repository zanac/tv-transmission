#include "TorrentFilesWindow.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TButton
#define Uses_TProgram
#define Uses_TEvent
#define Uses_TMenuItem
#define Uses_TMenu
#define Uses_TMenuPopup
#define Uses_TKeys
#include <tvision/tv.h>
#include <cstdio>
#include <map>

namespace {

// Local to this window: scoped to its own handleEvent (same reasoning
// as TrackerListWindow's cmRefreshTrackers/cmCloseTrackers).
constexpr ushort cmToggleWanted = 220;
constexpr ushort cmSelectAllFiles = 221;
constexpr ushort cmSelectNoneFiles = 222;
constexpr ushort cmSetPriorityLow = 223;
constexpr ushort cmSetPriorityNormal = 224;
constexpr ushort cmSetPriorityHigh = 225;
constexpr ushort cmCloseFiles = 226;

std::string formatFilePriority(int priority) {
    if (priority < 0) return tr(Str::PriorityLow);
    if (priority > 0) return tr(Str::PriorityHigh);
    return tr(Str::PriorityNormal);
}

// Splits a torrent file's path on '/' — the separator Transmission's
// own RPC always uses in "files[].name" regardless of the daemon's
// host OS. A file with no folder at all (the common case for a torrent
// that's just a flat pile of files) comes back as a single-element
// vector, which the tree-building below turns into exactly one file
// row with no folder wrapped around it — the same shape this window
// had before folder grouping existed.
std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= path.size(); i++) {
        if (i == path.size() || path[i] == '/') {
            if (i > start) parts.push_back(path.substr(start, i - start));
            start = i + 1;
        }
    }
    if (parts.empty()) parts.push_back(path); // defensive: an empty/odd name still gets a row
    return parts;
}

// A file, or a folder containing more of these (recursive via
// std::map, not std::vector, so the incomplete-type-at-definition
// issue a self-referential struct would otherwise have doesn't apply —
// map nodes are individually heap-allocated). `std::map` specifically
// (not `unordered_map`) so folder/file names within one directory come
// back out in alphabetical order for free, a reasonable default for a
// file-browser-shaped list with no explicit sort control of its own.
struct TreeNode {
    bool isFile = false;
    int fileIndex = -1;             // valid only when isFile
    std::map<std::string, TreeNode> children; // valid only when a folder
};

TreeNode buildFileTree(const std::vector<TorrentFile>& files) {
    TreeNode root;
    for (int i = 0; i < (int)files.size(); i++) {
        std::vector<std::string> parts = splitPath(files[i].name);
        TreeNode* cur = &root;
        for (size_t p = 0; p < parts.size(); p++) {
            bool isLast = (p + 1 == parts.size());
            TreeNode& child = cur->children[parts[p]];
            if (isLast) {
                child.isFile = true;
                child.fileIndex = i;
            }
            cur = &child;
        }
    }
    return root;
}

// Emits one row per node into `out` (folders before their own children,
// matching a normal file-browser's top-down order), returning the file
// indices found anywhere beneath `node` so the caller (a parent folder,
// or the top-level call) can fold them into its own aggregate — a
// folder's row ends up listing every descendant file's index, however
// many levels deep, not just its immediate children.
std::vector<int> emitTreeRows(const std::string& name, const TreeNode& node, int depth,
                               std::vector<FileTreeRow>& out) {
    if (node.isFile) {
        out.push_back({name, depth, false, {node.fileIndex}});
        return {node.fileIndex};
    }
    // The synthetic root itself (name == "") isn't a row — only real
    // folders (everything one level down from it or deeper) are.
    bool isRealFolder = !name.empty();
    size_t rowPos = out.size();
    if (isRealFolder) out.push_back({name, depth, true, {}}); // fileIndices filled in below

    std::vector<int> allIndices;
    for (auto& [childName, childNode] : node.children) {
        auto childIndices = emitTreeRows(childName, childNode, depth + (isRealFolder ? 1 : 0), out);
        allIndices.insert(allIndices.end(), childIndices.begin(), childIndices.end());
    }
    if (isRealFolder) out[rowPos].fileIndices = allIndices;
    return allIndices;
}

std::vector<FileTreeRow> buildFileTreeRows(const std::vector<TorrentFile>& files) {
    std::vector<FileTreeRow> rows;
    TreeNode root = buildFileTree(files);
    emitTreeRows("", root, 0, rows);
    return rows;
}

} // namespace

TorrentFilesWindow::TorrentFilesWindow(const TRect& bounds, TStringView title,
                                        int torrentId, TransmissionClient& client)
    : TWindowInit(&TDialog::initFrame),
      TDialog(bounds, title),
      torrentId_(torrentId), client_(client) {
    options |= ofCentered;

    TRect r = getExtent();
    r.grow(-1, -1);
    r.b.y -= 3; // room for the one remaining button row at the bottom —
                // "Toggle wanted" and the priority buttons are gone now
                // (double-click and the right-click context menu cover
                // both, see setCellActivateCallback()/
                // setRowContextCallback() below), so only "Select all"/
                // "Select none"/"Close" need a row

    grid_ = new TGridView(r, gvResizableColumns);
    insert(grid_);

    TGridColumn nameCol;
    nameCol.header = tr(Str::HeaderFileName);
    nameCol.width = 40;
    nameCol.minWidth = 15;
    grid_->addColumn(nameCol);

    TGridColumn sizeCol;
    sizeCol.header = tr(Str::HeaderFileSize);
    sizeCol.width = 10;
    sizeCol.minWidth = 6;
    sizeCol.align = TGridColumn::Align::Right;
    grid_->addColumn(sizeCol);

    TGridColumn doneCol;
    doneCol.header = tr(Str::HeaderFileProgress);
    doneCol.width = 6;
    doneCol.minWidth = 4;
    doneCol.align = TGridColumn::Align::Right;
    grid_->addColumn(doneCol);

    TGridColumn wantedCol;
    wantedCol.header = tr(Str::HeaderFileWanted);
    wantedCol.width = 8;
    wantedCol.minWidth = 5;
    grid_->addColumn(wantedCol);

    TGridColumn priorityCol;
    priorityCol.header = tr(Str::HeaderPriority);
    priorityCol.width = 8;
    priorityCol.minWidth = 6;
    grid_->addColumn(priorityCol);

    grid_->setCellTextCallback([this](int row, int col) -> std::string {
        if (row < 0 || row >= (int)rows_.size()) return "";
        const FileTreeRow& fr = rows_[row];

        // Every column past the name is an aggregate over fileIndices —
        // a single-element vector for a file row, so this naturally
        // reduces to "just that file's own value" there too, with no
        // separate file-vs-folder branch needed for columns 1-4.
        int64_t totalLength = 0, totalCompleted = 0;
        bool allWanted = true, noneWanted = true;
        bool allSamePriority = true;
        int firstPriority = fr.fileIndices.empty() ? 0 : files_[fr.fileIndices[0]].priority;
        for (int idx : fr.fileIndices) {
            const TorrentFile& f = files_[idx];
            totalLength += f.length;
            totalCompleted += f.bytesCompleted;
            if (f.wanted) noneWanted = false; else allWanted = false;
            if (f.priority != firstPriority) allSamePriority = false;
        }

        switch (col) {
            case 0: {
                std::string indent(fr.depth * 2, ' ');
                return indent + fr.name + (fr.isFolder ? "/" : "");
            }
            case 1: return formatSize(totalLength);
            case 2: {
                double pct = totalLength > 0 ? (double)totalCompleted / totalLength * 100.0 : 100.0;
                char buf[8];
                std::snprintf(buf, sizeof(buf), "%3.0f%%", pct);
                return buf;
            }
            case 3:
                // A tri-state checkbox rather than Yes/No/Mixed text —
                // "[-]" for partially wanted is the same convention as
                // a native tri-state checkbox control (checked/
                // unchecked/indeterminate), so it reads the same way at
                // a glance as the [X]/[ ] cases either side of it.
                if (allWanted) return "[X]";
                if (noneWanted) return "[ ]";
                return "[-]";
            case 4:
                return allSamePriority ? formatFilePriority(firstPriority) : tr(Str::ValueMixed);
        }
        return "";
    });

    // Without this, the focused-row/other-rows distinction relies
    // entirely on TListViewer's own default palette colors — which,
    // inside a TDialog, don't contrast enough to actually notice which
    // row is focused. Same fixed black-on-white-when-focused look
    // already used for the main torrent list and elsewhere in this app.
    grid_->setRowColorCallback([](int, bool focused) -> TColorAttr {
        return focused ? TColorAttr(0xF0) : TColorAttr(0x1F);
    });
    // Double-clicking a cell does something specific to WHICH column it
    // landed on, rather than the same thing regardless — the wanted
    // column toggles, the priority column cycles Low→Normal→High→Low
    // (Mixed, on a folder whose descendants disagree, resolves to Low
    // first — the same "ambiguous state picks the more conservative
    // option" rule toggleWantedForFocused() already uses). Any other
    // column is left alone; double-clicking the name or size doesn't do
    // anything special.
    grid_->setCellActivateCallback([this](int, int col) -> bool {
        if (col == 3) toggleWantedForFocused();
        else if (col == 4) cyclePriorityForFocused();
        return true; // consumed either way — this window has no
                     // RowActivateFn set to fall through to regardless
    });
    // Right-click: both actions again, plus the priority choices a
    // double-click's cycling can't reach directly (jumping straight to
    // High from Low, say) — see showContextMenuFor().
    grid_->setRowContextCallback([this](int row, TPoint pos) { showContextMenuFor(row, pos); });

    int buttonY = r.b.y + 1;
    int x = r.a.x;
    insert(new TButton(TRect(x, buttonY, x + 14, buttonY + 2), tr(Str::ButtonSelectAll), cmSelectAllFiles, bfNormal));
    x += 15;
    insert(new TButton(TRect(x, buttonY, x + 14, buttonY + 2), tr(Str::ButtonSelectNone), cmSelectNoneFiles, bfNormal));
    insert(new TButton(TRect(r.b.x - 10, buttonY, r.b.x, buttonY + 2), tr(Str::ButtonClose), cmCloseFiles, bfDefault));

    refresh();
}

void TorrentFilesWindow::refresh() {
    files_ = client_.getTorrentFiles(torrentId_);
    rows_ = buildFileTreeRows(files_);
    grid_->setRowCount((int)rows_.size());
    grid_->refresh();
}

void TorrentFilesWindow::toggleWantedForFocused() {
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)rows_.size()) return;
    const std::vector<int>& indices = rows_[row].fileIndices;
    if (indices.empty()) return;

    // If every file under this row (the row itself, for a plain file;
    // every descendant, for a folder) is already wanted, switch them
    // all off; otherwise (none, or a folder with a mix) switch them
    // all on. That second case is deliberate: toggling a folder whose
    // children disagree makes them agree first, rather than doing
    // something ambiguous like "toggle each independently" or picking
    // one child's state to mirror.
    bool allWanted = true;
    for (int idx : indices) if (!files_[idx].wanted) { allWanted = false; break; }
    client_.setFilesWanted(torrentId_, indices, !allWanted);
    refresh();
}

void TorrentFilesWindow::setPriorityForFocused(int priority) {
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)rows_.size()) return;
    const std::vector<int>& indices = rows_[row].fileIndices;
    if (indices.empty()) return;
    client_.setFilesPriority(torrentId_, indices, priority);
    refresh();
}

void TorrentFilesWindow::cyclePriorityForFocused() {
    int row = grid_->focusedRow();
    if (row < 0 || row >= (int)rows_.size()) return;
    const std::vector<int>& indices = rows_[row].fileIndices;
    if (indices.empty()) return;

    // Low(-1) -> Normal(0) -> High(1) -> Low(-1) — a folder whose
    // descendants disagree (Mixed) resolves to Low first, the same
    // "ambiguous state picks the more conservative option" rule
    // toggleWantedForFocused() already applies to a Mixed wanted state.
    int firstPriority = files_[indices[0]].priority;
    bool allSame = true;
    for (int idx : indices) if (files_[idx].priority != firstPriority) { allSame = false; break; }
    int next = -1;
    if (allSame) {
        if (firstPriority < 0) next = 0;
        else if (firstPriority == 0) next = 1;
        // else firstPriority > 0 (High): next stays -1 (Low)
    }
    client_.setFilesPriority(torrentId_, indices, next);
    refresh();
}

void TorrentFilesWindow::setAllWanted(bool wanted) {
    if (files_.empty()) return;
    std::vector<int> allIndices;
    allIndices.reserve(files_.size());
    for (int i = 0; i < (int)files_.size(); i++) allIndices.push_back(i);
    client_.setFilesWanted(torrentId_, allIndices, wanted);
    refresh();
}

void TorrentFilesWindow::showContextMenuFor(int /*row*/, TPoint screenPos) {
    // Same pattern as TorrentListWindow::showContextMenuFor() — see its
    // own comments for why the bounds are sized this way and why
    // execView()'s result gets re-posted as a command rather than acted
    // on directly here. Built fresh from tr()-translated strings (the
    // same ones the removed buttons used) rather than a static TMenu,
    // for the same reason every other menu in this app is: language can
    // change at runtime.
    TRect r(screenPos.x, screenPos.y, screenPos.x + 40, screenPos.y + 10);
    TMenu* menu = new TMenu(
        *new TMenuItem(tr(Str::ButtonToggleWanted), cmToggleWanted, kbNoKey) +
        *new TMenuItem(tr(Str::ButtonPriorityLow), cmSetPriorityLow, kbNoKey) +
        *new TMenuItem(tr(Str::ButtonPriorityNormal), cmSetPriorityNormal, kbNoKey) +
        *new TMenuItem(tr(Str::ButtonPriorityHigh), cmSetPriorityHigh, kbNoKey)
    );
    auto* popup = new TMenuPopup(r, menu);
    ushort chosen = TProgram::application->execView(popup);
    TObject::destroy(popup);
    if (chosen != 0) {
        TEvent e;
        e.what = evCommand;
        e.message.command = chosen;
        e.message.infoPtr = this;
        putEvent(e);
    }
}

void TorrentFilesWindow::handleEvent(TEvent& event) {
    TDialog::handleEvent(event);
    if (event.what != evCommand) return;
    switch (event.message.command) {
        case cmToggleWanted:      toggleWantedForFocused(); clearEvent(event); break;
        case cmSelectAllFiles:    setAllWanted(true);       clearEvent(event); break;
        case cmSelectNoneFiles:   setAllWanted(false);      clearEvent(event); break;
        case cmSetPriorityLow:    setPriorityForFocused(-1); clearEvent(event); break;
        case cmSetPriorityNormal: setPriorityForFocused(0);  clearEvent(event); break;
        case cmSetPriorityHigh:   setPriorityForFocused(1);  clearEvent(event); break;
        case cmCloseFiles:
            close();
            clearEvent(event);
            break;
    }
}

TDialog* createTorrentFilesWindow(int torrentId, const std::string& torrentName,
                                   TransmissionClient& client) {
    TRect r(0, 0, 90, 26);
    std::string shortName = truncateUtf8(torrentName, 30);
    char titleBuf[128];
    std::snprintf(titleBuf, sizeof(titleBuf), tr(Str::WindowTitleFiles), shortName.c_str());
    return new TorrentFilesWindow(r, titleBuf, torrentId, client);
}
