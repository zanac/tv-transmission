#include "Obfuscation.h"
#include <cstdlib>
#include <fstream>
#include <functional>

namespace {

// Same reasoning as machine-id-based key derivation elsewhere: stable
// per machine+user, not a secret in itself (an attacker with local
// access can read /etc/machine-id and $HOME just as easily as we can).
std::string keyMaterial() {
    std::string material;
    std::ifstream f("/etc/machine-id");
    if (f) std::getline(f, material);
    if (const char* home = std::getenv("HOME")) material += home;
    material += "tv-transmission-obfuscation-v1"; // app-specific salt, not a secret
    return material;
}

// Stretches keyMaterial() into a fixed-length byte key using std::hash
// repeatedly with an incrementing counter. This is NOT a cryptographic
// key-derivation function (std::hash makes no security guarantees) —
// it only needs to be deterministic (same machine/user -> same key
// every time) for the stated purpose above, not resistant to analysis.
std::string deriveKey(size_t length) {
    std::string material = keyMaterial();
    std::string key;
    key.reserve(length);
    size_t counter = 0;
    std::hash<std::string> hasher;
    while (key.size() < length) {
        size_t h = hasher(material + std::to_string(counter));
        for (size_t i = 0; i < sizeof(h) && key.size() < length; i++)
            key.push_back(static_cast<char>((h >> (i * 8)) & 0xFF));
        counter++;
    }
    return key;
}

std::string xorWithKey(const std::string& data, const std::string& key) {
    std::string out(data.size(), '\0');
    for (size_t i = 0; i < data.size(); i++)
        out[i] = data[i] ^ key[i % key.size()];
    return out;
}

const char* kBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::string& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= data.size()) {
        unsigned c0 = (unsigned char)data[i], c1 = (unsigned char)data[i+1], c2 = (unsigned char)data[i+2];
        out += kBase64Chars[c0 >> 2];
        out += kBase64Chars[((c0 & 0x3) << 4) | (c1 >> 4)];
        out += kBase64Chars[((c1 & 0xF) << 2) | (c2 >> 6)];
        out += kBase64Chars[c2 & 0x3F];
        i += 3;
    }
    size_t remaining = data.size() - i;
    if (remaining == 1) {
        unsigned c0 = (unsigned char)data[i];
        out += kBase64Chars[c0 >> 2];
        out += kBase64Chars[(c0 & 0x3) << 4];
        out += "==";
    } else if (remaining == 2) {
        unsigned c0 = (unsigned char)data[i], c1 = (unsigned char)data[i+1];
        out += kBase64Chars[c0 >> 2];
        out += kBase64Chars[((c0 & 0x3) << 4) | (c1 >> 4)];
        out += kBase64Chars[(c1 & 0xF) << 2];
        out += "=";
    }
    return out;
}

// Returns false if `text` isn't validly-formed base64 (used to detect
// a pre-existing plain-text password from before this feature existed
// — see the passwordObfuscated flag in Config.cpp).
bool base64Decode(const std::string& text, std::string& out) {
    auto valueOf = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    if (text.empty() || text.size() % 4 != 0) return false;
    out.clear();
    out.reserve(text.size() / 4 * 3);
    for (size_t i = 0; i < text.size(); i += 4) {
        int pad = 0;
        int vals[4];
        for (int j = 0; j < 4; j++) {
            char c = text[i + j];
            if (c == '=') { vals[j] = 0; pad++; }
            else {
                int v = valueOf(c);
                if (v < 0) return false; // not valid base64
                vals[j] = v;
            }
        }
        unsigned n = (vals[0] << 18) | (vals[1] << 12) | (vals[2] << 6) | vals[3];
        out += static_cast<char>((n >> 16) & 0xFF);
        if (pad < 2) out += static_cast<char>((n >> 8) & 0xFF);
        if (pad < 1) out += static_cast<char>(n & 0xFF);
    }
    return true;
}

} // namespace

std::string obfuscatePassword(const std::string& plaintext) {
    if (plaintext.empty()) return "";
    std::string key = deriveKey(plaintext.size());
    return base64Encode(xorWithKey(plaintext, key));
}

std::string deobfuscatePassword(const std::string& obfuscated) {
    if (obfuscated.empty()) return "";
    std::string decoded;
    if (!base64Decode(obfuscated, decoded)) return ""; // malformed: treat as unusable
    std::string key = deriveKey(decoded.size());
    return xorWithKey(decoded, key);
}
