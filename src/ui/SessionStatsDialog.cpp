#include "SessionStatsDialog.h"
#include "Strings.h"
#include "../TextUtil.h"

#define Uses_TDialog
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TEvent
#include <tvision/tv.h>

#include <cstdio>

namespace {

// Local to this dialog: scoped to its own handleEvent, same reasoning
// as similar local command constants elsewhere in this project.
constexpr ushort cmRefreshStats = 232;

// TStaticText with a settable body — see ServerSettingsDialog.cpp's own
// TResultLabel for why the base class doesn't have one, and the same
// newStr()/delete[] convention used there (and, before that, for a
// TView's own title elsewhere in this project). Duplicated here rather
// than shared: both are a few lines, file-local, and this project
// doesn't otherwise have a place for small view helpers used by
// exactly two unrelated dialogs.
class TResultLabel : public TStaticText {
public:
    TResultLabel(const TRect& bounds, TStringView aText) : TStaticText(bounds, aText) {}
    void setText(const std::string& s) {
        delete[] (char*)text;
        text = newStr(s.c_str());
        drawView();
    }
};

// Formats one row's worth of stats consistently across both the
// current-session and all-time sections, so a mismatch between the two
// (one showing "1.2 GB", the other "1234567890 B") can't happen from
// drifting formatting logic in two places.
std::string formatDownloaded(int64_t bytes) { return formatSize(bytes); }
std::string formatUploaded(int64_t bytes) { return formatSize(bytes); }
std::string formatActive(int64_t seconds) { return formatDuration(seconds); }

class SessionStatsDialogImpl : public TDialog {
public:
    SessionStatsDialogImpl(const TRect& bounds, TStringView title, TransmissionClient& client)
        : TWindowInit(&TDialog::initFrame), TDialog(bounds, title), client_(client) {}

    void applyStats(const SessionStats& s) {
        if (curDownloadLabel) curDownloadLabel->setText(formatDownloaded(s.currentDownloadedBytes));
        if (curUploadLabel) curUploadLabel->setText(formatUploaded(s.currentUploadedBytes));
        if (curActiveLabel) curActiveLabel->setText(formatActive(s.currentSecondsActive));
        if (allDownloadLabel) allDownloadLabel->setText(formatDownloaded(s.cumulativeDownloadedBytes));
        if (allUploadLabel) allUploadLabel->setText(formatUploaded(s.cumulativeUploadedBytes));
        if (allActiveLabel) allActiveLabel->setText(formatActive(s.cumulativeSecondsActive));
        if (startedLabel) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), tr(Str::LabelStatsStarted), s.cumulativeSessionCount);
            startedLabel->setText(buf);
        }
    }

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);
        if (event.what != evCommand) return;
        switch (event.message.command) {
            case cmRefreshStats: {
                bool ok = false;
                SessionStats s = client_.getSessionStats(&ok);
                if (ok) applyStats(s);
                clearEvent(event);
                break;
            }
            // No case for Close: its button uses the STANDARD cmCancel
            // (see createSessionStatsDialog() below) — TDialog's own
            // base handleEvent(), already called just above, turns that
            // into endModal(cmCancel) on its own. An earlier version of
            // this used a custom command handled with close() instead,
            // which calls TWindow::close() -> destroy(this) directly —
            // deleting this modal dialog WHILE its own execute() loop
            // (further up the very same call stack, inside execView())
            // was still running, a genuine use-after-free rather than
            // just "doesn't return the right value". Found by
            // recognizing the exact same pattern while fixing an
            // identical bug in TFolderBrowserDialog (see tvision-ext/'s
            // own README-equivalent comment there for the live-run
            // symptom that caught it), not from a report against this
            // dialog specifically.
        }
    }

    // Set by createSessionStatsDialog() once built.
    TResultLabel* curDownloadLabel = nullptr;
    TResultLabel* curUploadLabel = nullptr;
    TResultLabel* curActiveLabel = nullptr;
    TResultLabel* allDownloadLabel = nullptr;
    TResultLabel* allUploadLabel = nullptr;
    TResultLabel* allActiveLabel = nullptr;
    TResultLabel* startedLabel = nullptr;

private:
    TransmissionClient& client_;
};

} // namespace

TDialog* createSessionStatsDialog(const SessionStats& initial, TransmissionClient& client) {
    TRect r(0, 0, 50, 18);
    auto* dlg = new SessionStatsDialogImpl(r, tr(Str::DialogTitleSessionStats), client);
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 46, 3), tr(Str::LabelCurrentSession)));
    dlg->insert(new TStaticText(TRect(2, 3, 16, 4), tr(Str::LabelStatsDownloaded)));
    dlg->curDownloadLabel = new TResultLabel(TRect(18, 3, 46, 4), formatDownloaded(initial.currentDownloadedBytes));
    dlg->insert(dlg->curDownloadLabel);
    dlg->insert(new TStaticText(TRect(2, 4, 16, 5), tr(Str::LabelStatsUploaded)));
    dlg->curUploadLabel = new TResultLabel(TRect(18, 4, 46, 5), formatUploaded(initial.currentUploadedBytes));
    dlg->insert(dlg->curUploadLabel);
    dlg->insert(new TStaticText(TRect(2, 5, 16, 6), tr(Str::LabelStatsActive)));
    dlg->curActiveLabel = new TResultLabel(TRect(18, 5, 46, 6), formatActive(initial.currentSecondsActive));
    dlg->insert(dlg->curActiveLabel);

    dlg->insert(new TStaticText(TRect(2, 7, 46, 8), tr(Str::LabelAllTime)));
    dlg->insert(new TStaticText(TRect(2, 8, 16, 9), tr(Str::LabelStatsDownloaded)));
    dlg->allDownloadLabel = new TResultLabel(TRect(18, 8, 46, 9), formatDownloaded(initial.cumulativeDownloadedBytes));
    dlg->insert(dlg->allDownloadLabel);
    dlg->insert(new TStaticText(TRect(2, 9, 16, 10), tr(Str::LabelStatsUploaded)));
    dlg->allUploadLabel = new TResultLabel(TRect(18, 9, 46, 10), formatUploaded(initial.cumulativeUploadedBytes));
    dlg->insert(dlg->allUploadLabel);
    dlg->insert(new TStaticText(TRect(2, 10, 16, 11), tr(Str::LabelStatsActive)));
    dlg->allActiveLabel = new TResultLabel(TRect(18, 10, 46, 11), formatActive(initial.cumulativeSecondsActive));
    dlg->insert(dlg->allActiveLabel);

    char startedBuf[64];
    std::snprintf(startedBuf, sizeof(startedBuf), tr(Str::LabelStatsStarted), initial.cumulativeSessionCount);
    dlg->startedLabel = new TResultLabel(TRect(2, 11, 46, 12), startedBuf);
    dlg->insert(dlg->startedLabel);

    dlg->insert(new TButton(TRect(2, 14, 15, 16), tr(Str::ButtonRefresh), cmRefreshStats, bfDefault));
    dlg->insert(new TButton(TRect(17, 14, 30, 16), tr(Str::ButtonClose), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}
