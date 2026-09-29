#include "LanguageComboBox.h"
#include "Strings.h"

namespace {

// Built fresh for every LanguageComboBox instance: TComboBox takes
// ownership of whatever chain it's given and frees it in its own
// destructor (see the comment on TComboItem in src/tvision-ext/
// TComboBox.h), so two combo boxes can never share one chain.
//
// Order here is what TComboBox::focused indexes into, so it has to
// match Language's own numeric values (English=0 ... Russian=7) — see
// the LanguageComboBox constructor below, which passes `initial` cast
// straight to short as the focus index. Portuguese and
// PortugueseBrazilian are adjacent here, matching their own adjacent
// enum values (5, 6) — see Language's own comment in AppSettings.h for
// why Russian's value moved to make room.
TComboItem* buildLanguageItems() {
    return
        new TComboItem(tr(Str::LanguageEnglish), (ulong)Language::English,
        new TComboItem(tr(Str::LanguageItalian), (ulong)Language::Italian,
        new TComboItem(tr(Str::LanguageFrench),  (ulong)Language::French,
        new TComboItem(tr(Str::LanguageGerman),  (ulong)Language::German,
        new TComboItem(tr(Str::LanguageSpanish), (ulong)Language::Spanish,
        new TComboItem(tr(Str::LanguagePortuguese), (ulong)Language::Portuguese,
        new TComboItem(tr(Str::LanguagePortugueseBrazilian), (ulong)Language::PortugueseBrazilian,
        new TComboItem(tr(Str::LanguageRussian), (ulong)Language::Russian, nullptr))))))));
}

} // namespace

LanguageComboBox::LanguageComboBox(const TRect& bounds, Language initial)
    : TComboBox(bounds, buildLanguageItems(), static_cast<short>(initial)) {}
