#pragma once

#define Uses_TInputLine
#include <tvision/tv.h>
#include <cstring>
#include <string>

// A TInputLine that displays '*' instead of the actual characters
// typed, for password fields.
//
// tvision has no built-in masking flag on TInputLine (checked: nothing
// in dialogs.h/tinputli.cpp resembles one) — this works around that by
// temporarily swapping the public `data` member for a same-length
// string of asterisks only for the duration of the base class's own
// draw() call, then restoring the real value immediately after. Every
// other TInputLine method (getData/setData/the validator/handleEvent)
// keeps operating on the real, unmasked `data` as normal — only what
// gets painted to the screen is different. This reuses TInputLine::
// draw()'s own scrolling/selection-highlight math entirely instead of
// reimplementing it, which would be easy to get subtly wrong.
class PasswordInputLine : public TInputLine {
public:
    using TInputLine::TInputLine;

    void draw() override {
        std::string masked(std::strlen(data), '*');
        char* real = data;
        data = const_cast<char*>(masked.c_str());
        TInputLine::draw();
        data = real; // restored before `masked` goes out of scope
    }
};
