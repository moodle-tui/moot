#include "app.h"
#include "rlutil.h"

KeyDef getKeyDef(int key) {
    KeyDef keyDef;
    switch (key) {
        case 108: // l
        case KEY_RIGHT:
        case 10: // enter
            keyDef = KD_RIGHT;
            break;
        case 106: // j
        case KEY_DOWN:
            keyDef = KD_DOWN;
            break;
        case 104: // h
        case KEY_LEFT:
            keyDef = KD_LEFT;
            break;
        case 107: // k
        case KEY_UP:
            keyDef = KD_UP;
            break;
        case 0:
            if (getkey() == 91) {
                key = getkey();
                switch (key) {
                    case 67: // arrow key right
                        keyDef = KD_RIGHT;
                        break;
                    case 66: // arrow key down
                        keyDef = KD_DOWN;
                        break;
                    case 65: // arrow key up
                        keyDef = KD_UP;
                        break;
                    case 68: // arrow key left
                        keyDef = KD_LEFT;
                }
            }
            break;
        case 113: // q
            keyDef = KD_QUIT;
            break;
        case 115: // s
            keyDef = KD_DOWNLOAD;
            break;
    }
    return keyDef;
}

