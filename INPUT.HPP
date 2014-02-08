
#ifndef AH_INPUT
#define AH_INPUT

#ifdef AH_INPUTMANAGER
   #define _extrn
#else
   #define _extrn extern
#endif


#define INPUT_KEYBOARD     0x01
#define INPUT_JOYPAD       0x02
#define INPUT_MOUSE        0x04

// Global accessable variables.
_extrn int     keyboard;      // 1 if keyboard is detected.
_extrn int     joypad;        // 1 if joypad is detected.
_extrn int     mouse;         // 1 if mouse is detected.

// Keyboard stuff.
#define KEY_ESC         0x01
_extrn volatile int   keys;             // Number of currently pressed keys.
_extrn volatile int   key_pressed;      // Scan code of last pressed key.
_extrn volatile int   key_released;     // Scan code of last released key.
_extrn volatile char  key[0x80];        // Array of pressed keys.
void keyboard_wait(void);
void getkeyboard(int &x, int &y, int &fire);
void setkeys(int, int, int, int, int);

// Joystick stuff.
void joypad_wait(void);
int calibrate_joypad(void);
void getjoypad(int &x, int &y, int &fire);


// Mouse functions.
// Mouse sensitivity constants.
#define M_LOW        0
#define M_MEDIUM     1
#define M_HIGH       2
void getmousexy(int &b, int &x, int &y);
void setmousexy(int x, int y);
void setmouselimits(int x0, int y0, int x1, int y1);
void mouse_wait(void);
void setmousesens(int);
int getmousesens();
void getmouse(int &x, int &y, int &fire);

// General stuff.
void input_update(void);
void input_wait(void);
void input_fire(void);
void (*getdevice(int))(int &, int &, int &);
char *getdevicename(int);
int nextdevice(int);
int lockdevice(int);
int unlockdevice(int);
void initinputs(int);
void shutinputs(void);

#undef _extrn

#endif

