#ifndef GuiLib_h
#define GuiLib_h

#include "Arduino.h"

class GuiLib {
    public:
        GuiLib(int pin);
        Rectangle Rectangle();
    private:
        int _pin;
};

// UI Objects definition
class Rectangle {
    public:
        
    private:
};

#endif