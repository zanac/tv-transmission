#include "ConfirmDialog.h"

#define Uses_TProgram
#define Uses_TApplication
#define Uses_TDialog
#define Uses_TDeskTop
#define Uses_TButton
#define Uses_TStaticText
#define Uses_TRect
#define Uses_MsgBox
#include <tvision/tv.h>

bool confirmDefaultNo(const std::string& msg) {
    // Same geometry messageBox() itself uses: a 40x9 dialog centered on
    // the desktop (it builds the rect the same way before calling
    // messageBoxRect(), and the text area leaves the same margins).
    TRect r(0, 0, 40, 9);
    r.move((TProgram::deskTop->size.x - r.b.x) / 2,
           (TProgram::deskTop->size.y - r.b.y) / 2);

    auto* dialog = new TDialog(r, MsgBoxText::confirmText);
    dialog->insert(new TStaticText(
        TRect(3, 2, dialog->size.x - 2, dialog->size.y - 3), msg));

    auto* yes = new TButton(TRect(0, 0, 10, 2), MsgBoxText::yesText, cmYes, bfNormal);
    auto* no  = new TButton(TRect(0, 0, 10, 2), MsgBoxText::noText,  cmNo,  bfDefault);

    int total = yes->size.x + 2 + no->size.x;
    int x = (dialog->size.x - total) / 2;
    dialog->insert(yes);
    yes->moveTo(x, dialog->size.y - 3);
    dialog->insert(no);
    no->moveTo(x + yes->size.x + 2, dialog->size.y - 3);

    no->select();

    ushort result = TProgram::application->execView(dialog);
    TObject::destroy(dialog);
    return result == cmYes;
}
