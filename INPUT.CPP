
/*-------------------------------------------------------*/
/*  Input Manager                                        */
/*  [c]copyright 1995 by AlphaHelix                      */
/*                                                       */
/*                                                       */
/*-------------------------------------------------------*/

//#include <i86.h>
//#include <dos.h>
//#include <conio.h>

#include <stdio.h>

#include "SDL_BASE.HPP"
#include "error.hpp"
#define AH_INPUTMANAGER
#include "input.hpp"

// Globaler Schrott

#define MAXINPUTS          4     // Number of input devices supported.

SDL_Joystick* joystick = NULL;

// Next struct is a REGISTER structure for all input devices available.
// A device must be registered in this structure to get active.
// I know the idea is good but it's not implemented strict enough to earn
// the title CLEAN PROGRAMMING.
struct Device {
   int      locked;              // Device has been locked.
   char     *name;               // Device's name.
   void     (*getmotion)(int &, int &, int &);
};


//-------------------------------------------------------------------
// Keyboard interface routines.
//-------------------------------------------------------------------
// Private data.
// Defined movement keys.
static int  key_left = SDL_SCANCODE_LEFT;
static int  key_right = SDL_SCANCODE_RIGHT;
static int  key_up = SDL_SCANCODE_UP;
static int  key_down = SDL_SCANCODE_DOWN;
static int  key_fire = SDL_SCANCODE_SPACE;

/*---------------------------------------------------------
 Function: keyboard_wait

 Description:
  Waits until all keyboard keys have been released.
---------------------------------------------------------*/
void keyboard_wait(void)
{
	int x, y, fire;

	do
	{
		input_update();
		getkeyboard(x, y, fire);
	}
	while(x || y || fire);

}

void getkeyboard(int &x, int &y, int &fire)
{
	const Uint8 *keyState = SDL_GetKeyboardState(NULL);

   x = 0; y = 0; fire = 0;
   if (keyState[key_left]) x--;
   if (keyState[key_right]) x++;
   if (keyState[key_up]) y--;
   if (keyState[key_down]) y++;
   if (keyState[key_fire]) fire = 1;
}

void setkeys(int left, int right, int up, int down, int fire)
{
   key_left = left;
   key_right = right;
   key_up = up;
   key_down = down;
   key_fire = fire;
}

static void initkeyboard(void)
{
   keyboard = 1;     // Indicate "keyboard present".
}

static void shutkeyboard(void)
{
   keyboard = 0;
}


//-------------------------------------------------------------------
// Joystick interface routines.
//-------------------------------------------------------------------
#define JOYSTICK        0x201          // Joystick port.
#define JOYA_X          0x01
#define JOYA_Y          0x02
#define JOYA_B1         0
#define JOYA_B2         1

#define JOY_TIMEOUT     20000          // Joystick timeout value.

static   int   x0 = -32768/8, y0 = -32767/8, x1 = 32767/8, y1 = 32767/8;         // Joystick corner values.

static int measure(int &x, int &y, int &fire)
{
// Start new measure.
	x = SDL_JoystickGetAxis(joystick, 0);
	y = SDL_JoystickGetAxis(joystick, 1);

// Check FIRE button.
   fire = SDL_JoystickGetButton(joystick, JOYA_B1) | SDL_JoystickGetButton(joystick, JOYA_B2);
   
   return 0;

}

/*---------------------------------------------------------
 Function: joypad_wait

 Description:
 Waits until the joypad fire button has been released.
---------------------------------------------------------*/
void joypad_wait(void)
{
   int   i;
   int   x, y, fire;

   do {
		input_update();
      measure(x, y, fire);
   } while (fire);
   SDL_Delay(10);
}


void getjoypad(int &x, int &y, int &fire)
{
   int   dx, dy;

   x = y = 0;
   measure(dx, dy, fire);
   if (dx < x0) x--;
   if (dy < y0) y--;
   if (dx > x1) x++;
   if (dy > y1) y++;

}


int calibrate_joypad(void)
{
   int   x, y, fire;
   int   yes;

   yes = measure(x, y, fire);
   /*x0 = x - (x / 2);
   x1 = x + (x / 2);
   y0 = y - (y / 2);
   y1 = y + (y / 2);*/
   return yes;
}


void initjoypad(void)
{
	if (SDL_NumJoysticks() > 0) {

		joystick = SDL_JoystickOpen(0);

		if (joystick) {
			printf("Opened Joystick 0\n");
			printf("Name: %s\n", SDL_JoystickNameForIndex(0));
			printf("Number of Axes: %d\n", SDL_JoystickNumAxes(joystick));
			printf("Number of Buttons: %d\n", SDL_JoystickNumButtons(joystick));
			printf("Number of Balls: %d\n", SDL_JoystickNumBalls(joystick));
			
			joypad = 1;
		} else {
			printf("Couldn't open Joystick 0\n");
		}
	}

}

void shutjoypad(void)
{
	if (SDL_JoystickGetAttached(joystick)) {
		SDL_JoystickClose(joystick);
		
		joypad = 0;
	}
}


//-------------------------------------------------------------------
// Mouse interface routines.
//-------------------------------------------------------------------

static int  m_sensx;          // Mouse sensitivity x direction.
static int  m_sensy;          // Mouse sensitivity y direction.
static int  m_sens;           // Mouse sensitivity level. 0=low, 2=high.
static int m_limx0, m_limx1, m_limy0, m_limy1; //Current mouse limits

/*---------------------------------------------------------
 Function: getmouse

 Description:
 Get mouse button state and mouse position.
---------------------------------------------------------*/
void getmousexy(int &b, int &x, int &y)
{
   Uint32 buttonState = SDL_GetMouseState(&x, &y);

   b = buttonState & SDL_BUTTON(1); //Check if left mouse button is pressed.

	//printf("%d %d ---> %d\n", x, y, b);

   //Constrain mouse position
   if(x < m_limx0) x = m_limx0;
   else if(x > m_limx1) x = m_limx1;
   if(y < m_limy0) y = m_limy0;
   else if (y > m_limy1) y = m_limy1;
   
}

// Return the net mouse displacement since the last call to this function.
void getmousemotion(int &dx, int &dy)
{
	SDL_GetRelativeMouseState(&dx, &dy);
}



/*---------------------------------------------------------
 Function: setmousexy

 Description:
 Set mouse position
---------------------------------------------------------*/
void setmousexy(int x, int y)
{
	SDL_WarpMouseInWindow(NULL, x, y); //Warp mouse in the window of current mouse focus.
}

void setmouselimits(int x0, int y0, int x1, int y1)
{

// Set horizontal limits first.
   m_limx0 = x0;
   m_limx1 = x1;

// Go for the vertical limits now.
	m_limy0 = y0;
	m_limy1 = y1;
}


/*---------------------------------------------------------
 Function: mouse_wait

 Description:
 Waits until all mouse buttons have been released.
---------------------------------------------------------*/
void mouse_wait(void)
{
   int   x, y, b;

   getmousexy(b, x, y);
   while (b) 
	{
		input_update();
		getmousexy(b, x, y);
	}
}

void setmousesens(int sens)
{
   switch (sens) {
   case M_LOW:
      m_sensx = 2;
      m_sensy = 3;
      break;
   case M_MEDIUM:
      m_sensx = 1;
      m_sensy = 2;
      break;
   case M_HIGH:
      m_sensx = 0;
      m_sensy = 1;
   }
   m_sens = sens;
}

int getmousesens(void)
{
   return m_sens;
}

void getmouse(int &x, int &y, int &fire)
{
   int   dx, dy;

   x = y = 0;
   getmousexy(fire, dx, dy);
   getmousemotion(dx, dy);
   if (dx > m_sensx) x++;
   if (dx < -m_sensx) x--;
   if (dy > m_sensy) y++;
   if (dy < -m_sensy) y--;
}


void initmouse(void)
{
	mouse = 1;
	setmousesens(M_MEDIUM);
}

void shutmouse(void)
{
   if (mouse) {
      mouse = 0;
   }
}


//-------------------------------------------------------------------
// Input Manager code & data.
//-------------------------------------------------------------------

static int     ndevices;               // Number of devices available.
static Device  device[MAXINPUTS];      // Device description.

void input_update(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
			case SDL_MOUSEBUTTONDOWN:
			break;

			case SDL_MOUSEBUTTONUP:
			break;

			case SDL_WINDOWEVENT:

				switch(event.window.event)
				{
					case SDL_WINDOWEVENT_ENTER:
					window.enableMouseFocus();
					puts("Mouse focus gained!");
					break;

					case SDL_WINDOWEVENT_LEAVE:
					window.disableMouseFocus();
					puts("Mouse focus lost!");
					break;

					case SDL_WINDOWEVENT_FOCUS_LOST:
					window.disableInputFocus();
					puts("Input Focus lost!");
					break;

					case SDL_WINDOWEVENT_FOCUS_GAINED:
					window.enableInputFocus();
					puts("Input Focus gained!");
					break;
				}

			break;
		}
	}
}

/*---------------------------------------------------------
 Function: input_wait

 Description:
 Wait until all enabled input devices have been released.
---------------------------------------------------------*/
void input_wait(void)
{
   if (keyboard) keyboard_wait();
   if (mouse) mouse_wait();
   if (joypad) joypad_wait();
}

/*---------------------------------------------------------
 Function: input_fire

 Description:
 Waits until fire button on any enabled device has been
 pressed.
---------------------------------------------------------*/
void input_fire(void)
{
   int  j, k, m;
   int  x, y;
	int hasFocus = 0;
   k = j = m = 0;
   do {
		input_update();

      if (keyboard) getkeyboard(x, y, k);
      if (joypad) getjoypad(x, y, j);
      if (mouse) getmouse(x, y, m);
	} while ((!k && !j && !m) || !window.isFocus());

	if(m) puts("Mouse FIRE!");
}


// Some device handling routines.
void (*getdevice(int n))(int &, int &, int &)
{
   return device[n].getmotion;
}

char *getdevicename(int n)
{
   return device[n].name;
}

// Scan all available devices starting with device n
// and return the next UNLOCKED device greater than n.
// Returning -1 if none could be found.
int nextdevice(int n)
{
   int   i;

   i = ndevices;
   do {
      if (++n >= ndevices) n = 0;
   } while (--i && device[n].locked);
   if (device[n].locked) return -1;
   return n;
}

// Lock device n.
// returning >0 if ok. 0 if lock attempt failed.
int lockdevice(int n)
{
   if (n >= ndevices) return 0;
   device[n].locked = 1;
   return n;
}

int unlockdevice(int n)
{
   if (n >= ndevices) return 0;
   device[n].locked = 0;
   return n;
}

// Dummy input device. The NONE device.
void getnone(int &x, int &y, int &fire)
{
   x = y = fire = 0;
}

/*---------------------------------------------------------
 Function: *

 Description:
 Initialize the INPUT SYSTEM.
 Prepare all available inputs and fill in device structures.
---------------------------------------------------------*/
void initinputs(int input)
{
   ndevices = 0;

// Dummy input device.
   device[ndevices].locked = 0;
   device[ndevices].name = "None";
   device[ndevices].getmotion = getnone;
   ndevices++;

   keyboard = joypad = mouse = 0;

// KEYBOARD.
   if (input & INPUT_KEYBOARD) {
      initkeyboard();
      if (keyboard) {
         device[ndevices].locked = 0;
         device[ndevices].name = "Keyboard";
         device[ndevices].getmotion = getkeyboard;
         ndevices++;
      }
   }

// JOYPAD.
   if (input & INPUT_JOYPAD) {
      initjoypad();
      if (joypad) {
         device[ndevices].locked = 0;
         device[ndevices].name = "Joypad";
         device[ndevices].getmotion = getjoypad;
         ndevices++;
      }
   }

// MOUSE.
   if (input & INPUT_MOUSE) {
      initmouse();
      if (mouse) {
         device[ndevices].locked = 0;
         device[ndevices].name = "Mouse";
         device[ndevices].getmotion = getmouse;
         ndevices++;
      }
   }
}

void shutinputs(void)
{
   shutmouse();
   shutjoypad();
   shutkeyboard();
}



