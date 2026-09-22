#include "StatusPanel.h"
#include "Strings.h"

#define Uses_TStaticText
#define Uses_TSItem
#include <tvision/tv.h>
#include <vector>
#include <cstring>

namespace {

// Same idea as TSeedRatioRadio (TorrentDetailsWindow.cpp) and
// TTrackerPeerRadio (TrackerPeerWindow.h): a TCheckBoxes subclass that
// also calls back on every toggle, mouse or keyboard — TCluster (its
// own base) has no such hook built in.
class TLiveCheckBoxes : public TCheckBoxes {
public:
    using TCheckBoxes::TCheckBoxes;
    std::function<void()> onChanged;
    void press(int item) override {
        TCheckBoxes::press(item);
        if (onChanged) onChanged();
    }
};

// A dedicated, separate child view — not just a fill painted from the
// PANEL's own draw() (an earlier version did that, filling every row
// once at construction) — because that fill only ever reached rows
// tvision happened to ask THIS PANEL to redraw as a whole, which
// turned out to be just the rows a label/field/cluster actually sits
// on: every OTHER row (blank space around/below them) never got
// repainted again after the very first draw and reverted to whatever
// was behind this panel before it existed — found directly, reported
// as "background looks blue, not gray, and the input line is
// unreadable" — not a guess from re-reading the code. A view of its
// own gets asked to redraw its OWN full bounds on its OWN terms
// (tvision's normal per-view redraw, not a container painting beyond
// what it was actually invalidated for), so it doesn't depend on
// which child happened to trigger the redraw. Inserted FIRST, so
// every other child still draws on top of it normally.
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

StatusPanel::StatusPanel(const TRect& bounds, const TorrentFilter& initial,
                          std::function<void(const TorrentFilter&)> onFilterChanged)
    : TGroup(bounds), onFilterChanged_(std::move(onFilterChanged)) {
    growMode = gfGrowHiY; // full height of whatever TorrentListWindow gives it

    // 0x70 (gray background, black text) — matches a real TDialog's
    // own "Frame passive"/"StaticText" background exactly (see
    // mapColor()'s own comment below for how this was computed), not
    // guessed to merely look close.
    insert(new TPanelBackground(TRect(0, 0, size.x, size.y), TColorAttr(0x70)));

    // Every child below gets its own growMode = gfGrowHiX explicitly —
    // none of them had it until now, meaning none of them grew when
    // this panel itself was resized wider (only ever getting their
    // own final width at construction time, from this panel's own
    // size.x AT THAT MOMENT): reported directly, with a screenshot
    // showing exactly the gap this left on the right, after widening
    // the panel via drag. TPanelBackground (above, already correctly
    // covering the full width) was the one exception, since it
    // already needed this same fix for a different, earlier reason
    // (see its own constructor call above and TGridView's own
    // analogous fix, same underlying pattern).
    auto* nameLabel = new TStaticText(TRect(1, 1, size.x - 1, 2), tr(Str::LabelFilterName));
    nameLabel->growMode = gfGrowHiX;
    insert(nameLabel);
    nameField_ = new TInputLine(TRect(1, 2, size.x - 1, 3), 128);
    nameField_->growMode = gfGrowHiX;
    std::vector<char> nameBuf(129, 0);
    std::snprintf(nameBuf.data(), nameBuf.size(), "%s", initial.nameContains.c_str());
    nameField_->setData(nameBuf.data());
    insert(nameField_);

    auto* statusLabel = new TStaticText(TRect(1, 4, size.x - 1, 5), tr(Str::LabelFilterStatusSection));
    statusLabel->growMode = gfGrowHiX;
    insert(statusLabel);

    auto* boxes = new TLiveCheckBoxes(TRect(1, 5, size.x - 1, 9),
        new TSItem(tr(Str::TorrentStatusStopped),
        new TSItem(tr(Str::TorrentStatusChecking),
        new TSItem(tr(Str::TorrentStatusDownloading),
        new TSItem(tr(Str::TorrentStatusSeeding), nullptr)))));
    boxes->growMode = gfGrowHiX;
    // Four boxes, not seven — asked for directly, to make this panel
    // more compact: "Checking" here covers both Transmission's own
    // "queued to check" and "checking" states together, "Downloading"
    // both "queued to download" and "downloading", "Seeding" both
    // "queued to seed" and "seeding" — one checkbox setting both of
    // TorrentFilter's own underlying fields at once (readFilter()/
    // setFilter() below) rather than exposing each pair separately.
    // TorrentListWindow's own filter matching (TorrentListWindow.cpp)
    // needed no change at all for this — it already checks all seven
    // fields individually; they just always move in these three pairs
    // now; and AppSettings.h keeps all seven as well, unchanged, so an
    // existing settings.json full of the old, separate values still
    // loads exactly as it always did (initial checkbox state below
    // just ORs each pair together to decide whether the one combined
    // box reads as checked).
    ushort checked = (initial.showStopped ? 0x01 : 0) |
                     ((initial.showCheckWait || initial.showChecking) ? 0x02 : 0) |
                     ((initial.showDownloadWait || initial.showDownloading) ? 0x04 : 0) |
                     ((initial.showSeedWait || initial.showSeeding) ? 0x08 : 0);
    boxes->setData(&checked);
    boxes->onChanged = [this] { notifyChanged(); };
    insert(boxes);
    statusBoxes_ = boxes;

    selectNext(False);
}

TColorAttr StatusPanel::mapColor(uchar color) {
    // Every byte below is what actually arrives here from each
    // widget's own default (unmodified) mapColor(), given its OWN
    // local palette (cpStaticText/cpInputLine/cpCluster, tvision's
    // own tstatict.cpp/tinputli.cpp/tcluster.cpp) — computed by
    // chaining those through cpGrayDialog then cpAppColor (tvision's
    // own dialogs.h/app.h) exactly as a real TDialog would, not
    // approximated: a TDialog's own default palette IS this exact
    // chain, so hardcoding its end result here (rather than trying to
    // route through the chain itself, which failed for a flat
    // override attempted earlier — see StatusPanel.h's own comment)
    // reproduces it precisely.
    switch (color) {
        case 6:    return TColorAttr(0x70); // StaticText (both labels)
        case 0x13: return TColorAttr(0x1F); // InputLine normal/focused
        case 0x14: return TColorAttr(0x2F); // InputLine selected text
        case 0x15: return TColorAttr(0x1A); // InputLine scroll arrows
        case 0x10: return TColorAttr(0x30); // Cluster (checkbox) normal
        case 0x11: return TColorAttr(0x3F); // Cluster selected
        case 0x12: return TColorAttr(0x3E); // Cluster shortcut (~x~ letter)
        case 0x1F: return TColorAttr(0x38); // Cluster disabled
        default:   return TColorAttr(0x70); // safe fallback: matches the
                                             // background fill above,
                                             // so anything unanticipated
                                             // blends in rather than
                                             // standing out wrong
    }
}

void StatusPanel::handleEvent(TEvent& event) {
    // Same reasoning as ConnectionDialogImpl::handleEvent() (see its
    // own comment on this exact pattern): the focused child (nameField_,
    // a plain TInputLine) consumes and clears a keydown it handles
    // itself before TGroup::handleEvent() below returns, so both what
    // was focused and whether this was a keydown at all have to be
    // captured BEFORE that call, not read from the event afterward.
    TView* focusedBefore = current;
    bool wasKeyDown = (event.what == evKeyDown);
    TGroup::handleEvent(event);
    if (wasKeyDown && focusedBefore == nameField_) {
        notifyChanged();
    }
}

TorrentFilter StatusPanel::readFilter() const {
    TorrentFilter result;
    if (nameField_) {
        char buf[129] = {0};
        nameField_->getData(buf);
        result.nameContains = buf;
    }
    if (statusBoxes_) {
        ushort checked = 0;
        statusBoxes_->getData(&checked);
        result.showStopped = (checked & 0x01) != 0;
        result.showCheckWait = result.showChecking = (checked & 0x02) != 0;
        result.showDownloadWait = result.showDownloading = (checked & 0x04) != 0;
        result.showSeedWait = result.showSeeding = (checked & 0x08) != 0;
    }
    return result;
}

void StatusPanel::notifyChanged() {
    if (onFilterChanged_) onFilterChanged_(readFilter());
}

void StatusPanel::setFilter(const TorrentFilter& filter) {
    if (nameField_) {
        std::vector<char> buf(129, 0);
        std::snprintf(buf.data(), buf.size(), "%s", filter.nameContains.c_str());
        nameField_->setData(buf.data());
        nameField_->drawView();
    }
    if (statusBoxes_) {
        ushort checked = (filter.showStopped ? 0x01 : 0) |
                         ((filter.showCheckWait || filter.showChecking) ? 0x02 : 0) |
                         ((filter.showDownloadWait || filter.showDownloading) ? 0x04 : 0) |
                         ((filter.showSeedWait || filter.showSeeding) ? 0x08 : 0);
        statusBoxes_->setData(&checked);
        statusBoxes_->drawView();
    }
}
