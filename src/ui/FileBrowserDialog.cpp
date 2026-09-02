#include "FileBrowserDialog.h"
#include "Strings.h"

#define Uses_TButton
#define Uses_TEvent
#include <tvision/tv.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

struct Entry {
    std::string name; // display name; ".." for the parent-directory entry
    bool isDir;
};

// TStaticText's `text` member is protected and there's no public setter
// — this subclass adds one (managing the same newStr()/delete[]
// lifetime TStaticText's own constructor/destructor use) so the path
// label can be updated in place after navigating, instead of having to
// destroy and recreate it.
class MutableStaticText : public TStaticText {
public:
    MutableStaticText(const TRect& r, TStringView initial) : TStaticText(r, initial) {}

    void setText(const std::string& s) {
        delete[] text;
        text = newStr(s.c_str());
        drawView();
    }
};

class FileBrowserViewer : public TListViewer {
public:
    FileBrowserViewer(const TRect& r, TScrollBar* vScroll)
        : TListViewer(r, 1, nullptr, vScroll) {
        setRange(0);
    }

    void setEntries(std::vector<Entry> entries) {
        entries_ = std::move(entries);
        focused = 0;
        setRange((short)entries_.size());
        drawView();
    }

    const Entry* selectedEntry() const {
        if (focused < 0 || focused >= (int)entries_.size()) return nullptr;
        return &entries_[focused];
    }

    void getText(char* dest, short item, short maxLen) override {
        if (item < 0 || item >= (int)entries_.size()) { dest[0] = '\0'; return; }
        const Entry& e = entries_[item];
        std::snprintf(dest, maxLen, "%s%s", e.name.c_str(), e.isDir ? "/" : "");
    }

private:
    std::vector<Entry> entries_;
};

class FileBrowserDialogImpl : public TDialog {
public:
    FileBrowserDialogImpl(const TRect& r, TStringView title)
        : TWindowInit(&TDialog::initFrame), TDialog(r, title) {}

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what == evBroadcast &&
            event.message.command == cmListItemSelected &&
            event.message.infoPtr == viewer) {
            activateSelection();
            clearEvent(event);
        }
    }

    void navigateTo(const std::string& path) {
        std::vector<Entry> entries;
        if (path != "/") entries.push_back({"..", true});

        std::error_code ec;
        std::vector<Entry> dirs, files;
        for (const auto& de : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec)) {
            std::string name = de.path().filename().string();
            if (de.is_directory(ec)) {
                dirs.push_back({name, true});
            } else if (de.is_regular_file(ec)) {
                // Case-insensitive ".torrent" match.
                if (name.size() >= 8) {
                    std::string ext = name.substr(name.size() - 8);
                    std::transform(ext.begin(), ext.end(), ext.begin(),
                        [](unsigned char c) { return std::tolower(c); });
                    if (ext == ".torrent") files.push_back({name, false});
                }
            }
        }
        auto byName = [](const Entry& a, const Entry& b) { return a.name < b.name; };
        std::sort(dirs.begin(), dirs.end(), byName);
        std::sort(files.begin(), files.end(), byName);
        entries.insert(entries.end(), dirs.begin(), dirs.end());
        entries.insert(entries.end(), files.begin(), files.end());

        currentPath_ = path;
        pathLabel->setText(currentPath_);
        viewer->setEntries(std::move(entries));
    }

    std::string chosenPath; // set by activateSelection() when a file is picked
    FileBrowserViewer* viewer = nullptr;
    MutableStaticText* pathLabel = nullptr;

private:
    void activateSelection() {
        const Entry* e = viewer->selectedEntry();
        if (!e) return;
        if (e->isDir) {
            fs::path next = (e->name == "..")
                ? fs::path(currentPath_).parent_path()
                : fs::path(currentPath_) / e->name;
            navigateTo(next.empty() ? "/" : next.string());
        } else {
            chosenPath = (fs::path(currentPath_) / e->name).string();
            endModal(cmOK);
        }
    }

    std::string currentPath_;
};

} // namespace

TDialog* createFileBrowserDialog(const std::string& startDir) {
    TRect r(0, 0, 70, 20);
    auto* dlg = new FileBrowserDialogImpl(r, tr(Str::DialogTitleBrowseTorrent));
    dlg->options |= ofCentered;

    dlg->pathLabel = new MutableStaticText(TRect(2, 2, 68, 3), "");
    dlg->insert(dlg->pathLabel);

    TScrollBar* vScroll = new TScrollBar(TRect(67, 3, 68, 16));
    dlg->insert(vScroll);
    dlg->viewer = new FileBrowserViewer(TRect(2, 3, 67, 16), vScroll);
    dlg->insert(dlg->viewer);

    dlg->insert(new TButton(TRect(29, 17, 41, 19), tr(Str::ButtonCancel), cmCancel, bfDefault));

    dlg->navigateTo(startDir);
    dlg->selectNext(False);
    return dlg;
}

std::string fileBrowserDialogResult(TDialog* dlg) {
    if (auto* impl = dynamic_cast<FileBrowserDialogImpl*>(dlg))
        return impl->chosenPath;
    return "";
}
