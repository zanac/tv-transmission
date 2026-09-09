#include "ConnectionDialog.h"
#include "Strings.h"
#include "PasswordInputLine.h"
#include "../rpc/TransmissionClient.h"

#define Uses_TButton
#define Uses_TStaticText
#define Uses_TValidator
#define Uses_TRangeValidator
#define Uses_MsgBox
#include <tvision/tv.h>

#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Local command for the OK button — deliberately NOT cmOK. tvision's
// own TButton/TDialog machinery recognizes cmOK/cmCancel/cmYes/cmNo
// specially (auto-validating every field, then calling endModal()
// directly, before this dialog's own handleEvent() ever sees the
// click) — exactly the shortcut this button can no longer take, since
// confirming now needs a connection test to actually succeed first.
// Handled entirely by ConnectionDialogImpl::handleEvent() below, which
// only calls endModal(cmOK) itself once that test has passed. 62,
// grouped with TComboBox's own cmComboBoxItemAdded/cmComboBoxItemRemoved
// (60/61, see TComboBox.h) rather than up near 100 — App.h's own
// commands (cmAddTorrent and up) start exactly there, so this stays
// clear of them.
constexpr ushort cmTestConnectionAndOK = 62;

// Same reasoning as TorrentListWindow.cpp's own formatMessage() (not
// shared from there — this file has no other dependency on it): `fmt`
// is one of our own tr() strings with a single "%s" placeholder,
// `value` is a plain argument to it, not itself interpreted as a
// format string.
std::string formatMessage(const char* fmt, const std::string& value) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, value.c_str());
    return buf;
}

TInputLine* addField(TDialog* dlg, int y, const char* label,
                      const std::string& initialValue, int maxLen,
                      bool masked = false) {
    dlg->insert(new TStaticText(TRect(2, y, 24, y + 1), label));
    TInputLine* input = masked
        ? static_cast<TInputLine*>(new PasswordInputLine(TRect(24, y, 50, y + 1), maxLen))
        : new TInputLine(TRect(24, y, 50, y + 1), maxLen);

    // setData() does memcpy(data, rec, maxLen) on the buffer passed in:
    // it must be at least maxLen readable bytes long, so no direct
    // c_str() of a shorter string.
    std::vector<char> buf(maxLen + 1, 0);
    std::snprintf(buf.data(), buf.size(), "%s", initialValue.c_str());
    input->setData(buf.data());

    dlg->insert(input);
    return input;
}

// Same safety note as addField()'s own buf above — setData() always
// reads dataSize()-1 bytes from whatever's passed in (see
// tinputli.cpp), regardless of how short the actual string is. 256 is
// comfortably more than any field in this dialog's own maxLen (128 at
// most), so this one buffer size covers all of them.
void setFieldText(TInputLine* field, const std::string& value) {
    if (!field) return;
    std::vector<char> buf(256, 0);
    std::snprintf(buf.data(), buf.size(), "%s", value.c_str());
    field->setData(buf.data());
    field->drawView();
}

std::string getFieldText(TInputLine* field) {
    if (!field) return "";
    char buf[256];
    field->getData(buf);
    return buf;
}

// Builds the server-name combo's item chain from `current.servers`'
// own keys (alphabetical — see AppSettings.h's own comment on why
// servers is a std::map), and works out which one to focus initially:
// current.activeServer if it's actually one of them, otherwise
// whichever ends up first.
TComboItem* buildServerItems(const AppSettings& current, short& focusedIndex) {
    TComboItem* head = nullptr;
    TComboItem* tail = nullptr;
    focusedIndex = 0;
    short idx = 0;
    for (const auto& [name, profile] : current.servers) {
        (void)profile;
        TComboItem* item = new TComboItem(name.c_str(), 0, nullptr);
        if (name == current.activeServer) focusedIndex = idx;
        if (head == nullptr) head = tail = item;
        else { tail->next = item; tail = item; }
        idx++;
    }
    return head;
}

// A plain TDialog everywhere else in this file's own instantiation
// (`new TDialog(...)`) would have done — subclassed for three reasons,
// all handled in the one handleEvent() override below: reacting to the
// server-name combo's own cmComboBoxSelectionChanged broadcast (loading
// that server's saved details into the other fields), showing a
// confirmation for its cmComboBoxItemAdded/cmComboBoxItemRemoved
// broadcasts ("[+]"/"[-]"), and running a connection test before
// actually confirming the dialog (see cmTestConnectionAndOK above).
// Kept local to this file rather than given its own header: nothing
// outside createConnectionDialog() below ever needs to name this type.
class ConnectionDialogImpl : public TDialog {
public:
    ConnectionDialogImpl(const TRect& r, const char* title) noexcept :
        TWindowInit(&TDialog::initFrame), TDialog(r, title) {}

    ConnectionDialogFields* fields = nullptr;
    const AppSettings* current = nullptr;

    void handleEvent(TEvent& event) override {
        TDialog::handleEvent(event);

        if (event.what == evBroadcast && fields != nullptr &&
            event.message.infoPtr == fields->serverName) {
            if (event.message.command == cmComboBoxSelectionChanged) {
                std::string name = fields->serverName->editText();
                auto it = current->servers.find(name);
                if (it != current->servers.end()) {
                    // Only loads the OTHER fields when the picked name
                    // already has a saved profile — a name just typed
                    // and added via "[+]", never yet confirmed with OK,
                    // has none, and leaving the other fields alone in
                    // that case means whatever the user might already
                    // be filling in for that new server doesn't get
                    // wiped out from under them.
                    setFieldText(fields->host, it->second.host);
                    setFieldText(fields->port, std::to_string(it->second.port));
                    setFieldText(fields->user, it->second.user);
                    setFieldText(fields->password, it->second.password);
                }
            } else if (event.message.command == cmComboBoxItemAdded) {
                messageBox(formatMessage(tr(Str::MsgServerAdded), fields->serverName->lastChangedValue()),
                           mfInformation | mfOKButton);
            } else if (event.message.command == cmComboBoxItemRemoved) {
                messageBox(formatMessage(tr(Str::MsgServerRemoved), fields->serverName->lastChangedValue()),
                           mfInformation | mfOKButton);
            }
        }

        if (event.what == evCommand && event.message.command == cmTestConnectionAndOK) {
            // Runs every field's own TValidator (the refresh interval
            // and port's TRangeValidator — see createConnectionDialog()
            // below) before even attempting a test — the same check a
            // plain cmOK button gets automatically from tvision itself,
            // which this button no longer does simply by being cmOK.
            // valid()'s own argument only matters for telling cmCancel
            // apart from everything else (cmCancel skips validation
            // entirely) — cmOK here is just conventional, not literally
            // what confirming will end up doing next.
            if (valid(cmOK)) {
                TransmissionClient testClient(getFieldText(fields->host),
                                               std::atoi(getFieldText(fields->port).c_str()),
                                               getFieldText(fields->user),
                                               getFieldText(fields->password));
                bool ok = false;
                testClient.getSessionLimits(&ok);
                if (ok) {
                    endModal(cmOK);
                } else {
                    messageBox(formatMessage(tr(Str::MsgConnectionTestFailed), testClient.lastError()),
                               mfError | mfOKButton);
                }
            }
            clearEvent(event);
        }
    }
};

} // namespace

TDialog* createConnectionDialog(const AppSettings& current, ConnectionDialogFields& fields) {
    TRect r(0, 0, 60, 20);
    auto* dlg = new ConnectionDialogImpl(r, tr(Str::DialogTitleConnection));
    dlg->options |= ofCentered;

    dlg->insert(new TStaticText(TRect(2, 2, 24, 3), tr(Str::LabelLanguage)));
    fields.language = new LanguageComboBox(TRect(24, 2, 50, 3), current.language);
    dlg->insert(fields.language);

    dlg->insert(new TStaticText(TRect(2, 4, 24, 5), tr(Str::LabelServerName)));
    short focusedIndex = 0;
    TComboItem* serverItems = buildServerItems(current, focusedIndex);
    fields.serverName = new TComboBox(TRect(24, 4, 50, 5), serverItems, focusedIndex);
    fields.serverName->setEditable(true);
    if (serverItems == nullptr) {
        // No servers configured yet (a fresh install) — nothing for
        // the combo to focus, so its own editText() would otherwise
        // start out empty with no hint of what to type. Not saved
        // anywhere until "[+]" is used or OK is confirmed with this
        // still showing.
        fields.serverName->setEditText(tr(Str::DefaultServerName));
    }
    dlg->insert(fields.serverName);

    fields.refreshInterval = addField(dlg, 6, tr(Str::LabelRefreshSeconds),
        std::to_string(current.refreshIntervalSeconds), 10);
    // TRangeValidator filters non-digit keystrokes as they're typed (see
    // isValidInput() in tvision's tvalidat.cpp) and blocks confirming the
    // dialog with an out-of-range value (shows its own "Value not in the
    // range X to Y" messageBox) — real validation, not just parsing
    // whatever ends up in the field after the fact.
    fields.refreshInterval->setValidator(new TRangeValidator(1, 86400)); // up to 24h
    const ServerProfile& active = current.activeProfile();
    fields.host = addField(dlg, 8, tr(Str::LabelHost), active.host, 128);
    fields.port = addField(dlg, 10, tr(Str::LabelPort), std::to_string(active.port), 10);
    fields.port->setValidator(new TRangeValidator(1, 65535)); // valid TCP port range
    fields.user = addField(dlg, 12, tr(Str::LabelUser), active.user, 128);
    fields.password = addField(dlg, 14, tr(Str::LabelPassword), active.password, 128, /*masked=*/true);

    static_cast<ConnectionDialogImpl*>(dlg)->fields = &fields;
    static_cast<ConnectionDialogImpl*>(dlg)->current = &current;

    // A blank row (16) between the last field and the buttons, rather
    // than the buttons sitting immediately under Password. The OK
    // button uses cmTestConnectionAndOK, not cmOK — see its own
    // definition above for why — but keeps bfDefault (Enter still
    // reaches it the same way a real cmOK button would).
    dlg->insert(new TButton(TRect(20, 17, 30, 19), tr(Str::ButtonOK), cmTestConnectionAndOK, bfDefault));
    dlg->insert(new TButton(TRect(32, 17, 42, 19), tr(Str::ButtonCancel), cmCancel, bfNormal));

    dlg->selectNext(False);
    return dlg;
}

AppSettings connectionDialogResult(const ConnectionDialogFields& fields, const AppSettings& current) {
    AppSettings result = current;
    char buf[256];

    if (fields.refreshInterval) {
        fields.refreshInterval->getData(buf);
        int v = std::atoi(buf);
        if (v > 0) result.refreshIntervalSeconds = v;
    }
    if (fields.language) {
        result.language = fields.language->language();
    }

    if (fields.serverName) {
        std::string activeName = fields.serverName->editText();

        // Rebuilt from the combo's own current list (TComboBox::
        // allValues()) rather than editing current.servers in place —
        // a name removed via "[-]" needs to disappear from the saved
        // set too, and only what's still IN the combo (not what's
        // missing from it) can tell us that.
        result.servers.clear();
        for (const std::string& name : fields.serverName->allValues()) {
            if (name == activeName) continue; // filled in below, from the fields actually shown
            auto it = current.servers.find(name);
            if (it != current.servers.end()) result.servers[name] = it->second;
            // A name with no prior profile (added via "[+]" but never
            // confirmed while active) gets no entry — see
            // ConnectionDialogImpl::handleEvent()'s own comment on why
            // that's the same case where the other fields are left
            // alone rather than blanked.
        }

        if (!activeName.empty()) {
            ServerProfile profile;
            if (fields.host) { fields.host->getData(buf); profile.host = buf; }
            if (fields.port) {
                fields.port->getData(buf);
                int v = std::atoi(buf);
                if (v > 0) profile.port = v;
            }
            if (fields.user) { fields.user->getData(buf); profile.user = buf; }
            if (fields.password) { fields.password->getData(buf); profile.password = buf; }
            result.servers[activeName] = profile;
            result.activeServer = activeName;
        }
    }

    return result;
}
