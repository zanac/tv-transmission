#pragma once
#include <string>

// Yes/No confirmation popup like tvision's own messageBox(...,
// mfConfirmation | mfYesButton | mfNoButton), except "No" is the
// default button — focused when the popup opens and triggered by Enter —
// instead of "Yes" (messageBox always focuses the first button and has
// no way to change that). Used for the destructive actions (remove from
// list / delete with files), where an Enter pressed out of habit or by
// accident should cancel, not confirm. Esc also cancels.
// Returns true only if "Yes" was explicitly chosen.
bool confirmDefaultNo(const std::string& msg);
