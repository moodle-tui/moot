#ifndef __MSG_H
#define __MSG_H

typedef const char cchar;

// notifyBig prints box with msg in the middle of the screen, waits for keypress,
// then disappears. Box borders are printed in the color that is passed.
void msgBig(cchar *msg, int color);

// notifySmall prints msg in bottom left corner. msg text is printed in the color
// that is passed.
void msgSmall(cchar *msg, int color);

// printErr prints msg in red background, waits for key press, then disappears
void msgErr(cchar *msg);

#endif // __MSG_H
