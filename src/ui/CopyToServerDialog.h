#pragma once

#include <string>
#include <vector>

// Modal dialog used by Torrent -> "Add to another server..." (Shift+F2):
// a TComboBox listing `serverNames` plus Add / Cancel buttons. Returns
// the chosen server's name, or an empty string if the user cancelled
// (or `serverNames` is empty). `subject` is what is being copied — a
// torrent name, or a short "N torrents" summary — shown in the label.
std::string chooseTargetServer(const std::vector<std::string>& serverNames,
                               const std::string& subject);
