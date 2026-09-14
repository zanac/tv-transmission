#pragma once
#include <string>

// Lightweight obfuscation for the RPC password stored in settings.json —
// NOT real encryption. The XOR key is derived deterministically from
// this machine's /etc/machine-id and the user's $HOME, entirely from
// data an attacker with the same level of access already has (anyone
// who can read settings.json on this machine can derive the same key
// and reverse this in a few lines of code). What this DOES protect
// against: the password showing up in clear if you `cat` the file,
// paste it into a support ticket, or someone glances at your screen.
// It is not a substitute for the file's 0600 permissions, which remain
// the actual access control.
//
// For real protection, the correct mechanism is an OS-level secret
// store (e.g. libsecret/Secret Service on Linux) — not implemented
// here because it requires a running keyring daemon, which typically
// isn't available on the headless/SSH-managed servers this app is
// often used from (see the discussion in README.md).
std::string obfuscatePassword(const std::string& plaintext);
std::string deobfuscatePassword(const std::string& obfuscated);
