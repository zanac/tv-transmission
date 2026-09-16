#include "TFolderBrowserDialog.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TInputLine
#define Uses_TEvent
#define Uses_TKeys
#include <tvision/tv.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {

constexpr ushort cmSelectFolder = 1;

std::string joinPath(const std::string& base, const std::string& name) {
    return (fs::path(base) / fs::path(name)).string();
}

// "/" for anything one level below the filesystem root, or already at
// it — a single well-defined floor rather than needing a separate
// "already at root" check at every call site.
std::string parentPath(const std::string& path) {
    fs::path p(path);
    if (p.empty()) return fs::current_path().string();
    fs::path parent = p.parent_path();
    if (parent.empty())
        return p.root_path().empty() ? p.string() : p.root_path().string();
    return parent.string();
}

bool isRootPath(const std::string& path) {
    fs::path p(path);
    return !p.root_path().empty() && p.lexically_normal() == p.root_path();
}

// Every readable subdirectory of `path`, alphabetically, name only (not
// full paths) — files are silently skipped, this only ever lists
// directories. `*ok` is false only if `path` itself couldn't be opened
// at all (doesn't exist, or no permission to list it); an individual
// entry that can't be stat()'d (a broken symlink, a permission quirk on
// just that one item) is skipped rather than failing the whole listing
// — one bad entry shouldn't hide every other one in the same directory.
std::vector<std::string> listSubdirectories(const std::string& path, bool* ok) {
    std::vector<std::string> result;
    *ok = false;
    std::error_code ec;
    fs::directory_iterator it(
        fs::path(path),
        fs::directory_options::skip_permission_denied,
        ec);
    if (ec) return result;
    *ok = true;
    for (const auto& entry : it) {
        std::error_code typeError;
        if (!entry.is_directory(typeError) || typeError) continue;
        result.push_back(entry.path().filename().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace

class TFolderBrowserDialogImpl : public TDialog {
public:
    TFolderBrowserDialogImpl(const TRect& bounds, TStringView title, const TFolderBrowserLabels& labels)
        : TWindowInit(&TDialog::initFrame), TDialog(bounds, title), labels_(labels) {}

    // Re-lists `path` and, if that succeeds, adopts it as the current
    // directory — the path field and grid both updated to match. On
    // failure, deliberately leaves currentPath_ (the last successfully
    // listed directory) untouched: reverting the path field's own
    // display back to it, rather than leaving it showing whatever
    // unreadable text was just typed/navigated to, and switching the
    // grid to a single row explaining why (see TFolderBrowserLabels::
    // unreadableDirectory's own comment) instead of just going blank
    // with no explanation.
    void navigateTo(const std::string& path) {
        bool ok = false;
        std::vector<std::string> dirs = listSubdirectories(path, &ok);
        if (!ok) {
            listOk_ = false;
            setPathFieldText(currentPath_);
            if (grid_) {
                grid_->setRowCount(1);
                grid_->refresh();
            }
            return;
        }
        currentPath_ = fs::path(path).lexically_normal().string();
        subdirs_ = std::move(dirs);
        listOk_ = true;
        atRoot_ = isRootPath(currentPath_);
        setPathFieldText(currentPath_);
        if (grid_) {
            grid_->setRowCount((atRoot_ ? 0 : 1) + (int)subdirs_.size());
            grid_->refresh();
        }
    }

    void setPathFieldText(const std::string& s) {
        if (!pathField) return;
        std::vector<char> buf(s.size() + 1, 0);
        std::snprintf(buf.data(), buf.size(), "%s", s.c_str());
        pathField->setData(buf.data());
        pathField->drawView();
    }

    std::string pathFieldText() const {
        if (!pathField) return "";
        char buf[4096] = {0};
        pathField->getData(buf);
        return buf;
    }

    // Intercepted BEFORE TDialog::handleEvent() runs, not after —
    // otherwise the base class's own "Enter presses whatever button has
    // bfDefault" handling would already have fired (and cleared the
    // event) by the time this got a chance to check whose focus it
    // was, since a normal TInputLine doesn't consume Enter on its own.
    void handleEvent(TEvent& event) override {
        if (event.what == evKeyDown && event.keyDown.keyCode == kbEnter && current == pathField) {
            navigateTo(pathFieldText());
            clearEvent(event);
            return;
        }
        TDialog::handleEvent(event);
        if (event.what != evCommand) return;
        switch (event.message.command) {
            case cmSelectFolder:
                // Validates/adopts whatever's currently in the path
                // field (which might not match currentPath_ yet — the
                // user could have typed a new path and clicked Select
                // directly, without pressing Enter to navigate there
                // first) before confirming. Left open, with the grid's
                // own "(cannot read this directory)" row as the only
                // feedback, if that fails — no separate messageBox on
                // top of it (see this class's own doc comment in the
                // header on staying to one way of showing "that didn't
                // work").
                navigateTo(pathFieldText());
                if (listOk_) endModal(cmOK);
                clearEvent(event);
                break;
            // No case for Cancel: its button uses the STANDARD cmCancel
            // (see createFolderBrowserDialog() below), which TDialog's
            // own base handleEvent() — already called just above —
            // turns into endModal(cmCancel) on its own. An earlier
            // version of this used a custom command handled with
            // close() instead, which doesn't actually end a MODAL
            // dialog's own execView() loop the way endModal() does —
            // Cancel appeared to work (the dialog visually went away)
            // but execView() itself never returned, silently breaking
            // whatever the caller was waiting to do next. Caught from a
            // live run where the caller's own follow-up (reopening "Add
            // torrent") never happened after Cancel — not from reading
            // the code alone.
        }
    }

    TInputLine* pathField = nullptr;
    TGridView* grid_ = nullptr;
    std::string currentPath_;
    std::vector<std::string> subdirs_;
    bool atRoot_ = false;
    bool listOk_ = true;
    TFolderBrowserLabels labels_;
};

TDialog* createFolderBrowserDialog(const std::string& initialPath, const TFolderBrowserLabels& labels) {
    TRect r(0, 0, 55, 18);
    auto* dlg = new TFolderBrowserDialogImpl(r, labels.title.c_str(), labels);
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 9, 3), labels.pathLabel.c_str()));
    dlg->pathField = new TInputLine(TRect(9, 2, 51, 3), 4096);
    dlg->insert(dlg->pathField);

    TRect gridRect(2, 4, 51, 13);
    dlg->grid_ = new TGridView(gridRect, 0);
    TGridColumn nameCol;
    nameCol.header = "Folder";
    nameCol.width = gridRect.b.x - gridRect.a.x;
    nameCol.sortable = false;
    dlg->grid_->addColumn(nameCol);
    dlg->insert(dlg->grid_);

    // No setRowColorCallback() here anymore — TGridView's own default
    // (white-on-blue/black-on-white-when-focused) already matches what
    // this used to set explicitly, since that became the generic
    // widget's own fallback instead of something every caller needed to
    // repeat.

    TFolderBrowserDialogImpl* implPtr = dlg;
    dlg->grid_->setCellTextCallback([implPtr](int row, int) -> std::string {
        if (!implPtr->listOk_) return implPtr->labels_.unreadableDirectory;
        if (!implPtr->atRoot_ && row == 0) return "..";
        int idx = row - (implPtr->atRoot_ ? 0 : 1);
        if (idx < 0 || idx >= (int)implPtr->subdirs_.size()) return "";
        return implPtr->subdirs_[idx] + "/";
    });
    dlg->grid_->setRowActivateCallback([implPtr](int row) {
        if (!implPtr->listOk_) return;
        if (!implPtr->atRoot_ && row == 0) {
            implPtr->navigateTo(parentPath(implPtr->currentPath_));
            return;
        }
        int idx = row - (implPtr->atRoot_ ? 0 : 1);
        if (idx < 0 || idx >= (int)implPtr->subdirs_.size()) return;
        implPtr->navigateTo(joinPath(implPtr->currentPath_, implPtr->subdirs_[idx]));
    });

    dlg->insert(new TButton(TRect(2, 15, 15, 17), labels.selectButton.c_str(), cmSelectFolder, bfDefault));
    dlg->insert(new TButton(TRect(17, 15, 30, 17), labels.cancelButton.c_str(), cmCancel, bfNormal));

    dlg->selectNext(False);

    std::string start = initialPath;
    if (start.empty()) {
        std::error_code ec;
        fs::path cwd = fs::current_path(ec);
        start = ec ? fs::path(".").string() : cwd.string();
    }
    dlg->navigateTo(start);
    if (!dlg->listOk_) {
        std::error_code ec;
        fs::path cwd = fs::current_path(ec);
        if (!ec)
            dlg->navigateTo(cwd.root_path().empty()
                ? cwd.string()
                : cwd.root_path().string());
    }

    return dlg;
}

std::string folderBrowserResult(TDialog* dialog) {
    auto* impl = dynamic_cast<TFolderBrowserDialogImpl*>(dialog);
    return impl ? impl->currentPath_ : "";
}
