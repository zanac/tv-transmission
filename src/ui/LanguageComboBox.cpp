#include "LanguageComboBox.h"
#include "Strings.h"

namespace {

// Built fresh for every LanguageComboBox instance: TComboBox takes
// ownership of whatever chain it's given and frees it in its own
// destructor (see the comment on TComboItem in tvision's dialogs.h), so
// two combo boxes can never share one chain.
//
// Order here is what TComboBox::focused indexes into, so it has to
// match Language's own numeric values (English=0 ... Spanish=4) — see
// the LanguageComboBox constructor below, which passes `initial` cast
// straight to short as the focus index.
TComboItem* buildLanguageItems() {
    return
        new TComboItem(tr(Str::LanguageEnglish), (ulong)Language::English,
        new TComboItem(tr(Str::LanguageItalian), (ulong)Language::Italian,
        new TComboItem(tr(Str::LanguageFrench),  (ulong)Language::French,
        new TComboItem(tr(Str::LanguageGerman),  (ulong)Language::German,
        new TComboItem(tr(Str::LanguageSpanish), (ulong)Language::Spanish, nullptr)))));
}

} // namespace

LanguageComboBox::LanguageComboBox(const TRect& bounds, Language initial)
    : TComboBox(bounds, buildLanguageItems(), static_cast<short>(initial)) {}
