
/*-------------------------------------------------------*/
/*                                                       */
/* [c]copyright 1996 by AlphaHelix                       */
/*                                                       */
/* This is the mainmodule of RAVAGE. And I wrote it.     */
/* Yes. I wrote it on my own.                            */
/*                                                       */
/*-------------------------------------------------------*/

/*
 Ultimate DEBUG MASTER switches.
 These switches must be set for EACH and EVERY module in the project.

 DEBUG   : Enables extended runtime checks.
           - Memory check (out of memory, unfreed memory, messed up
             memory structure, trying to free unallocated memory).
           - Parameters in range check.
           - In play DEBUG keys are enabled.
             (a = bypass frame limiter)
             (s = enter\leave single step mode)
             (. = do single step)
             (d = execute INT 3 exeption ----> ENTER DEBUGGER)
             (i = toggle invincible on/off.)

 RECORDER: Enables access to the game recorder for DEMO recording.
 SHAREWARE:Build SHAREWARE VERSION of RAVAGE.
*/

#include <malloc.h>
#include <setjmp.h>
#include <sys\types.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <climits>

#define AH_MAINMODULE
#include "error.hpp"
#include "memory.hpp"
#include "WINDOWS.HPP"
#include "SDL_BASE.HPP"

#include "input.hpp"
#include "diskio.hpp"
#include "ravage.hpp"
#include "sound.hpp"

#include "XLIB.HPP"


#ifdef RAVAGE_PORT
#include "types.h"

#include "objects.hpp"
#endif


// Some variables.
#ifdef SHAREWARE
const char copyright[] = "RAVAGE shareware 1.1 [c]copyright 1996 by AlphaHelix.";
#else
const char copyright[] = "RAVAGE registered 1.1 [c]copyright 1996 by AlphaHelix.";
#endif

using namespace std;



/*---------------------------------------------------------
 Function: timer

 Description:
 This is the all important timer function.
 This function will be called 60 times a second and
 updates the "timer_ticks" variable.
 "timer_ticks" is then used in the rest of the program
 to synchronize game speed and screen output to 30
 frames per second.
---------------------------------------------------------*/
volatile int  timer_ticks;
void timer(void)
{
   timer_ticks++;
}

/*---------------------------------------------------------
 Function: cmdline

 Description:
 Concatenate all command arguments to one string.
 So easy search can be done now.
---------------------------------------------------------*/
#define CMDLEN       40             // Max allowed length of command line.
static char    cmd[CMDLEN];         // Command line arguments.
int incmd(char *text)
{
   return (int)strstr(cmd, text);
}

static int cmdnumber(char *text, int def)
{
   char  *i = strstr(cmd, text);

   if (!i) return def;
   return i[strlen(text)] - '0';
}

static void cmdline(int argc, char *argv[])
{
   int   i;

// Concatenate all command strings together in 'cmd'.
   cmd[0] = '\0';			// Clear string.
   for (i = 1; i < argc; i++) {
      if (strlen(cmd)+strlen(argv[i]) < CMDLEN)
	 strcat(cmd, argv[i]);
   }
   strlwr(cmd);

}

/*---------------------------------------------------------
 Function: loadsettings & savesettings

 Description:
 These functions are used to save\load the configuration
 the user has choosen.
---------------------------------------------------------*/
static void loadsettings(char *file)
{
   myFile   myfile;

// Fill in settings structure with FACTORY DEFAULTS.

// Sound settings.
   settings.voices = 2;
   settings.sound_device = 1;
   settings.mix_freq = 44100;
   settings.mod_vol = 10;
   settings.wav_vol = 30;

// Control settings.
// mouse.
   settings.msens = M_MEDIUM;
// keyboard
   settings.key[0] = SDL_SCANCODE_LEFT;      
   settings.key[1] = SDL_SCANCODE_RIGHT;     
   settings.key[2] = SDL_SCANCODE_UP;     
   settings.key[3] = SDL_SCANCODE_DOWN;      
   settings.key[4] = SDL_SCANCODE_SPACE;      

   settings.device[0] = 1;       // Player 1 input: Keyboard.
   settings.device[1] = 3;       // Player 2 input: Mouse.

// Graphix
   settings.frate = 1;
   settings.shadows = 0;
   settings.cpu_usage = 0;

// Try to load configuration file.
   if (myfile.open(file)) {
      myfile.read(&settings, sizeof(Settings));
      myfile.close();
   }
}

static void savesettings(char *file)
{
   myFile   myfile;

   if (!myfile.creat(file)) return;
   myfile.write(&settings, sizeof(Settings));
   myfile.close();

}

/*---------------------------------------------------------
 Function: powerdown

 Description:
 Function called upon exit of the program.
 Does all the needed housekeeping AFTER the
 registered exit functions (reg_exit) have been
 called.
---------------------------------------------------------*/
static void powerdown(void)
{
#ifdef DEBUG
   memory_usage();
#endif
   #ifdef RAVAGE_PORT
   cout << "Object peak usage: " << object_usage() << " of "
        << OBJ_MAXOBJECTS << " used." << endl;
   cout << "Weapon peak usage: " << weapon_usage() << " of "
        << G_MAXWEAPONS << " used." << endl;
   cout << "Enemy peak usage: " << enemy_usage() << " of "
        << G_MAXENEMIES << " used." << endl;
#endif
   _heapmin();

   SDL_Quit();

   cout << endl;
   cout << "Thank you for playing RAVAGE." << endl << endl;
}


/*---------------------------------------------------------
 Function: powerup

 Description:
 Initialize the system.
 - SDL
 - memory manager
 - timer
 - sound card
 - graphics
 - inputs (keyboard, mouse, joystick)
---------------------------------------------------------*/
static void powerup(void)
{
	// Init SDL

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) != 0) 
	{
		fprintf(stderr, "\nUnable to initialize SDL:  %s\n", SDL_GetError());
		return;
	}

	atexit(powerdown);

// Grow heap.
   {
		char *ptr;
      ptr = new char[MEMNEEDED*1024];
      if (ptr == 0) error("Not enough memory available.");
      delete []ptr;
   }

// Init File Sytem.
   initfilesystem(); reg_exit(shutfilesystem);

// Load configuration.
	loadsettings(CONFIGFILE);

// Initialize Inputs.
   initinputs(INPUT_KEYBOARD | INPUT_MOUSE | INPUT_JOYPAD);

   reg_exit(shutinputs);
   if (!lockdevice(settings.device[0])) {
      settings.device[0] = nextdevice(0);
      lockdevice(settings.device[0]);
   }
   if (!lockdevice(settings.device[1])) {
      settings.device[1] = nextdevice(0);
      lockdevice(settings.device[1]);
   }

// Define keys according to loaded settings.
   setkeys(settings.key[0], settings.key[1],
            settings.key[2], settings.key[3], settings.key[4]);

   if (joypad) cout << "Joypad detected." << endl;
   if (mouse) cout << "Mouse detected." << endl;

// Init Sound system.
   initsound(); reg_exit(shutsound);
   s_setmodvol(settings.mod_vol);
   s_setwavvol(settings.wav_vol);
   cout << "Sound driver: " << s_drivername() << endl;
	
	// Enter graphics.
   initgraphics(); reg_exit(shutgraphics);

// Init OBJECT system.
   initobjects(); reg_exit(shutobjects);
   shadow_level(settings.shadows);
// Init game classes.
   initgclass(); reg_exit(shutgclass);


// No game is in progress.
   gstate.ready = 0;

// Process command line.
// First (DJ)Ammann cheats.
   cheatlevel = 0;
   if (incmd("/at")) cheatlevel |= CHEAT_INVUL;   // absolut toedlich.
   if (incmd("/vv")) cheatlevel |= CHEAT_MONEY;   // vertammi vertori.
// Level jumping.
   start_stage = cmdnumber("/l", 1);
   if (start_stage > MAXLEVELS) error("/l parameter out of range.");

#ifdef DEBUG
   if (incmd("/motz")) start_stage = 99;
#endif

#ifdef RECORDER
   recorder = 0;
   if (incmd("/rec")) recorder = 1;
#endif

}

 int sdl_drawsprite(Sprite *sprite, int x, int y, int n)
 {
 }

 int sdl_reportsprite(Sprite *sprite)
 {
	printf("Sprite(%p):\n", sprite);
	printf("header: %d\n", sprite->header);
	printf("xs_data: %ld\n", sprite->xs_data);
	printf("ys_data: %ld\n", sprite->ys_data);
	printf("fsize: %ld\n", sprite->fsize);
	printf("xs: %ld\n", sprite->xs);
	printf("ys: %ld\n", sprite->ys);
	printf("frames: %ld\n", sprite->frames);
	printf("speed: %ld\n", sprite->speed);
	printf("x_core: %ld\n", sprite->x_core);
	printf("y_core: %ld\n", sprite->y_core);
	printf("xs_core: %ld\n", sprite->xs_core);
	printf("ys_core: %ld\n", sprite->ys_data);

	SDLSprite spriteTest;

	render.clear();

	spriteTest.load(sprite, false);
	printf("Texture width: %d\n", spriteTest.getTexture()->getWidth());
	
	render.x_drawsprite(&spriteTest, 0, 0, -1);

	render.update();

	input_fire();
	SDL_Delay(5000);
	spriteTest.destroy();
	return 0;
}

int sdl_drawpcx(char *name)
{
	myFile myFile;
	SDLTexture pcxTexture;
	
	if(!myFile.open(name)) error("Unable to open file.", name);

	SDL_RenderClear(render.getBase());
	pcxTexture.loadMemory(myFile.getMemory(), myFile.getsize());
	
	render.drawTexture(&pcxTexture, 0, 0, 50, 0, 100, 120);

	render.update();

	myFile.close();

	int startTicks = SDL_GetTicks();
	while(SDL_GetTicks() - startTicks < 2000)
		{
			input_update();
	}

	pcxTexture.destroy();

	return 0;
}

int sdl_ask_abort(void)
{
   Font     *tiny;
   Sprite   *escbox;
   SDLSprite escSprite;
	int      x, y, fire0 = 0, fire1 = 0;

   tiny = new Font("fonts\\tiny.fnt");
   escbox = (Sprite *) loadfile("pic\\escbox.spr", NULL);
	escSprite.load(escbox, false);

   y = (YMAX-escbox->ys)/2;

	render.x_drawsprite(&escSprite, (XMAX-escbox->xs)/2, y, 0);
   y += 15;

   tiny->vanilla_c(XMAX/2, y, "Press ESC again to abort the game.", 12);
   y += 15;
   tiny->vanilla_c(XMAX/2, y, "Fire to continue.", 15);

	render.update();

   unloadfile(escbox);
   delete tiny;
	escSprite.destroy();

	input_fire();

	return 0;
}

void sdl_showorder(void)
{
   void  *f;
   Font  *tiny, *blue;
   char  *text;
   int   x, y;
	int size = 0;

// Load MOD.
   s_loadmod("mods\\menu.uni");
// Start music and fade in screen.
   s_startmod();

   tiny = new Font("fonts\\tiny.fnt");
   blue = new Font("fonts\\blue.fnt");

   f = loadfile("pic\\order.pcx", &size);

   text = (char *)loadfile("stuff\\order1.txt", NULL);

   render.clear(0, 0, 0);

   showpcx(f, 20, size);
   blue->print_c(XMAX/2, 0, "ORDER RAVAGE NOW");
   x = 0; y = 30;
   beauty(tiny, x, y, text);
   unloadfile(text);

   render.update();
   fadein(getpcxpal());

   input_fire();
   fadeout(FADE_PALETTE);

   text = (char *)loadfile("stuff\\order2.txt", NULL);

   render.clear(0, 0, 0);
   showpcx(f, 20, size);
   blue->print_c(XMAX/2, 0, "HOW TO ORDER");
   x = 0; y = 30;
   beauty(tiny, x, y, text);
   unloadfile(text);

   render.update();
   fadein(getpcxpal());

   unloadfile(f);

   input_fire();
   fadeout(FADE_ALL);

   delete blue;
   delete tiny;

   s_unloadmod();
}

int main(int argc, char *argv[])
{
   int   request;          // Returned request from MAIN MENU.

// Set up error handling LONG JUMP.
   if (setjmp(_err_jmp)) {
// If an error occurs we will jump here.
      reg_down();          // Properly shut down the system.
      post_error();        // Post error processing.
      exit(1);
   }

// Display some message.
   cout << endl << copyright << endl << endl;
   cout << "RAVAGE SDL" << endl;
   cout << endl;

// Prepare to access the command line.
   cmdline(argc, argv);

// Start the system.
   powerup();

// Open FilePool.
   filepool[0].open("RAV0.DAT");
   filepool[1].open("RAV1.DAT");
   filepool[2].open("RAV2.DAT");
   filepool[3].open("RAV3.DAT");

	// Test music
	// Load MOD.

   //s_loadmod("level1\\LEVEL1.UNI");
	// Start music
   //s_startmod();
	//s_setmodvol(64);
	
	//Load palette
	void *p = (void *)loadfile("demo1\\DEMO1.CPA", NULL);
	palette.set(p);
	delete p;

	//Test sprites
	Sprite   *label;        // Player score LABELS.

	/*label = (Sprite *)loadfile("pic\\ready.spr");
	sdl_reportsprite(label);
	label = (Sprite *)loadfile("pic\\ravage.spr");
	sdl_reportsprite(label);
		label = (Sprite *)loadfile("pic\\pointer.spr");
	sdl_reportsprite(label);
	label = (Sprite *)loadfile("pic\\save.spr");
	sdl_reportsprite(label);
		label = (Sprite *)loadfile("pic\\stuff.spr");
	sdl_reportsprite(label);
		label = (Sprite *)loadfile("pic\\main.spr");
	sdl_reportsprite(label);
		label = (Sprite *)loadfile("pic\\players.spr");
	sdl_reportsprite(label);
	
	sdl_drawpcx("pic\\STORY.PCX");
	sdl_drawpcx("pic\\TITLE.PCX");
	sdl_drawpcx("pic\\ORDER.PCX");
	*/
	/*sdl_ask_abort();

	cout << "Press fire to exit." << endl;
	input_wait();
   input_fire();
	fadeout(FADE_ALL);
	sdl_showorder();*/

	/*WavePool weapWaves;
	weapWaves.load("level6\\LEVEL6.CWV");
	cout << "Wave samples: " << weapWaves.nsamples << endl;
	for(int x = 0; x < weapWaves.nsamples; x++)
	{
		SDL_Delay(500);
		weapWaves.play(x % weapWaves.nsamples);
		cout << "Sample: " << x % weapWaves.nsamples << endl;
		sound_Report();
		s_update();
	}*/


// Show TITLE screen.
   title();

// Play loop.
   request = M_QUIT;
   do {
      switch (request) {
      case M_PLAY:
// Enable RECORDER if needed.
         #ifdef RECORDER
            if (recorder) rec_enable();
         #endif
         play(0);    // Play NEW game.
         break;
      case M_LOAD:
         play(1);    // Load game.
         break;
      case M_SOUND:  // Reinitialize Sound System.
         s_setmodvol(settings.mod_vol);
         s_setwavvol(settings.wav_vol);
         break;
      }
      if (request == M_SOUND) {
         request = menu(SOUNDMENU, 0);
      } else {
         request = menu(MAINMENU, 0);
      }
   } while (request != M_QUIT);

// Close FilePool.
   filepool[3].close();
   filepool[2].close();
   filepool[1].close();
   filepool[0].close();

// Done for the game. Back to OS.
   reg_down();                // Execute registered shutdown functions.
   savesettings(CONFIGFILE);  // Save current game settings.

   exit(0);                   // Exit nicely.
   return 0;
}