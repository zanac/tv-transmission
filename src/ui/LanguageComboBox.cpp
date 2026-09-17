#include "LanguageComboBox.h"
#include "Strings.h"

namespace {

// Built fresh for every LanguageComboBox instance: TComboBox takes
// ownership of whatever chain it's given and frees it in its own
// destructor (see the comment on TComboItem in src/tvision-ext/
// TComboBox.h), so two combo boxes can never share one chain.
//
// Order here is what TComboBox::focused indexes into, so it has to
// match Language's own numeric values (English=0 ... Portuguese=7) —
// see the LanguageComboBox constructor below, which passes `initial`
// cast straight to short as the focus index. Portuguese is listed
// LAST, out of alphabetical/grouped order with its closer sibling
// PortugueseBrazilian, because its own enum value (7) is appended at
// the end for the same settings.json backward-compatibility reason
// (see Language's own comment in AppSettings.h) — the combo's own
// item order has to match, not read more naturally.
TComboItem* buildLanguageItems() {
    return
        new TComboItem(tr(Str::LanguageEnglish), (ulong)Language::English,
        new TComboItem(tr(Str::LanguageItalian), (ulong)Language::Italian,
        new TComboItem(tr(Str::LanguageFrench),  (ulong)Language::French,
        new TComboItem(tr(Str::LanguageGerman),  (ulong)Language::German,
        new TComboItem(tr(Str::LanguageSpanish), (ulong)Language::Spanish,
        new TComboItem(tr(Str::LanguagePortugueseBrazilian), (ulong)Language::PortugueseBrazilian,
        new TComboItem(tr(Str::LanguageRussian), (ulong)Language::Russian,
        new TComboItem(tr(Str::LanguagePortuguese), (ulong)Language::Portuguese, nullptr))))))));
}

} // namespace

LanguageComboBox::LanguageComboBox(const TRect& bounds, Language initial)
    : TComboBox(bounds, buildLanguageItems(), static_cast<short>(initial)) {}
