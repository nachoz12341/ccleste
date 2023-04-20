/*---------------------------------------------------------------------------------

---------------------------------------------------------------------------------*/
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include "celeste.h"
#include "tilemap.h"

//DS Specific headers
#include <nds.h>
#include <gl2d.h>
#include <maxmod9.h>

//Resources
#include "atlas.h"
#include "font.h"
#include "soundbank.h"
#include "soundbank_bin.h"
#include <inttypes.h>

//Texture is 128x128 and tiles are 8x8
glImage  Atlas[(128/8) * (128/8)];  
int AtlasTextureID;

mm_sound_effect snd[64];// = {NULL};
char* mus[6] = {NULL};

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

#define PICO8_W 128
#define PICO8_H 128

static const float scale_x = 2.0f;
static const float scale_y = 1.5f;

static _Bool enable_screenshake = 1;
static _Bool paused = 0;
static _Bool running = 1;
static void* initial_game_state = NULL;
static void* game_state = NULL;
static void mainLoop(void);

static char osd_text[200] = "";
static int osd_timer = 0;

static uint16 buttons_state = 0;

#define LOGLOAD(w) printf("loading %s...", w)
#define LOGDONE() printf("done\n")

static unsigned short palette[16];

enum {
	PSEUDO_BTN_SAVE_STATE = 6,
	PSEUDO_BTN_LOAD_STATE = 7,
	PSEUDO_BTN_EXIT = 8,
	PSEUDO_BTN_PAUSE = 9,
};

static inline unsigned short getcolor(char idx) {
	unsigned short c = palette[idx%16];
	glColorTableEXT(0,0,16,0,0,palette);
	return c;
}


//TODO LOAD SOUNDS
static void LoadData(void) {
	LOGLOAD("atlas.bmp");
	AtlasTextureID = 
		glLoadTileSet( Atlas,				// pointer to glImage array
					   8,					// sprite width
					   8,					// sprite height
					   128,					// bitmap width
					   128,					// bitmap height
					   GL_RGB16,			// texture type for glTexImage2D() in videoGL.h 
					   TEXTURE_SIZE_128,	// sizeX for glTexImage2D() in videoGL.h
					   TEXTURE_SIZE_128,	// sizeY for glTexImage2D() in videoGL.h
					   GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|TEXGEN_OFF|GL_TEXTURE_COLOR0_TRANSPARENT, // param for glTexImage2D() in videoGL.h
					   256,					// Length of the palette to use (256 colors)
					   (u16*)atlasPal,		// Load our 256 color tiles palette
					   (u8*)atlasBitmap		// image data generated by GRIT
					 );
	LOGDONE();
	
	LOGLOAD("font.bmp");
	//loadbmpscale("font.bmp", &font);
	LOGDONE();

	//Load Sound effects
	static const char sndnum[] = {0,1,2,3,4,5,6,7,8,9,13,14,15,16,23,35,37,38,40,50,51,54,55};
	static const char sndids[] = {SFX_SND0,SFX_SND1,SFX_SND2,SFX_SND3,SFX_SND4,SFX_SND5,SFX_SND6,SFX_SND7,SFX_SND8,SFX_SND9,SFX_SND13,SFX_SND14,SFX_SND15,SFX_SND16,SFX_SND23,SFX_SND35,SFX_SND37,SFX_SND38,SFX_SND40,SFX_SND50,SFX_SND51,SFX_SND54,SFX_SND55};
	for (int id = 0; id < sizeof sndnum; id++) {
		char fname[20];
		sprintf(fname, "snd%i.wav", sndnum[id]);
		LOGLOAD(fname);
		mm_sound_effect temp = {{sndids[id]},(int)(1.0f * (1<<10)),0,255,255};
		snd[sndnum[id]] = temp;
		mmLoadEffect(sndids[id]);
		LOGDONE();
	}
	/*static const char musnum[] = {0,10,30,40};
	static const char musids[] = {SFX_MUS0,SFX_MUS10,SFX_MUS30,SFX_MUS40};
	for (int iid = 0; iid < sizeof musids; iid++) {
		int id = musnum[iid];
		char fname[20];
		sprintf(fname, "mus%i.wav", id);
		LOGLOAD(fname);
		mmLoadEffect(musids[iid]);
		char path[4096];
		GetDataPath(path, sizeof path, fname);
		mus[id/10] = //Mix_LoadMUS(path);
		if (!mus[id/10]) {
			ErrLog("mus%i: failed to load: %s\n", id, "error");//Mix_GetError());
		}
		LOGDONE();
	}*/
}

static void ResetPalette(void) {
	memcpy(palette, atlasPal, sizeof palette);
}

static int gettileflag(int tile, int flag) {
	return tile < sizeof(tile_flags)/sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

static void p8_line(int x0, int y0, int x1, int y1, unsigned char color) {
	#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
	CLAMP(x0,0,SCREEN_WIDTH);
	CLAMP(y0,0,SCREEN_HEIGHT);
	CLAMP(x1,0,SCREEN_WIDTH);
	CLAMP(y1,0,SCREEN_HEIGHT);

	unsigned short realcolor = getcolor(color);
	#undef CLAMP
  	
	#define PLOT(x,y) do {                                               \
     glBoxFilled(x*scale_x,y*scale_y,x*scale_x+scale_x,y*scale_y+scale_y,realcolor); \
	} while (0)

	int sx, sy, dx, dy, err, e2;
	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	if (!dx && !dy) return;

	if (x0 < x1) sx = 1; else sx = -1;
	if (y0 < y1) sy = 1; else sy = -1;
	err = dx - dy;
	if (!dy && !dx) return;
	else if (!dx) { //vertical line
		for (int y = y0; y != y1; y += sy) PLOT(x0,y);
	} else if (!dy) { //horizontal line
		for (int x = x0; x != x1; x += sx) PLOT(x,y0);
	} while (x0 != x1 || y0 != y1) {
		PLOT(x0, y0);
		e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
	#undef PLOT
}

static void p8_rectfill(int x0, int y0, int x1, int y1, int col) {
	int w = (x1 - x0 + 1)*scale_x;
	int h = (y1 - y0 + 1)*scale_y;
	if (w > 0 && h > 0) {
		glBoxFilled(x0*scale_x, y0*scale_y, x1*scale_x, y1*scale_y, getcolor(col));
	}
}

//TODO PRINT TEXT
static void p8_print(const char* str, int x, int y, int col) {
	/*for (char c = *str; c; c = *(++str)) {
		c &= 0x7F;
		SDL_Rect srcrc = {8*(c%16), 8*(c/16)};
		srcrc.x *= scale;
		srcrc.y *= scale;
		srcrc.w = srcrc.h = 8*scale;
		
		SDL_Rect dstrc = {x*scale, y*scale, scale, scale};
		Xblit(font, &srcrc, screen, &dstrc, col, 0,0);
		x += 4;
	}*/
}

//on-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc)
static void OSDset(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(osd_text, sizeof osd_text, fmt, ap);
	osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
	printf("%s\n", osd_text);
	osd_timer = 30;
	va_end(ap);
}

static void OSDdraw(void) {
	if (osd_timer > 0) {
		--osd_timer;
		const int x = 4;
		const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); //disappear by going below the screen
		p8_rectfill(x-2, y-2, x+4*strlen(osd_text), y+6, 6); //outline
		p8_rectfill(x-1, y-1, x+4*strlen(osd_text)-1, y+5, 0);
		p8_print(osd_text, x, y, 7);
	}
}

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) 
{
	static int camera_x = 0, camera_y = 0;
	if (!enable_screenshake) {
		camera_x = camera_y = 0;
	}

	va_list args;
	int ret = 0;
	va_start(args, call);
	
	#define   INT_ARG() va_arg(args, int)
	#define  BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
	#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
	#define RET_BOOL(_b) RET_INT(!!(_b))

	switch (call) {
		case CELESTE_P8_MUSIC: { //music(idx,fade,mask)
			/*int index = INT_ARG();
			int fade = INT_ARG();
			int mask = INT_ARG();

			(void)mask; //we do not care about this since sdl mixer keeps sounds and music separate
			//TODO
			/*if (index == -1) { //stop playing
				Mix_FadeOutMusic(fade);
				current_music = NULL;
			} else if (mus[index/10]) {
				Mix_Music* musi = mus[index/10];
				current_music = musi;
				Mix_FadeInMusic(musi, -1, fade);
			}*/
		} break;
		case CELESTE_P8_SPR: { //spr(sprite,x,y,cols,rows,flipx,flipy)
			int sprite = INT_ARG();
			int x = INT_ARG();
			int y = INT_ARG();
			int cols = INT_ARG();
			int rows = INT_ARG();
			int flipx = BOOL_ARG();
			int flipy = BOOL_ARG();

			(void)cols;
			(void)rows;

			assert(rows == 1 && cols == 1);

			if (sprite >= 0) {
				uint16 flip = GL_FLIP_NONE;
				if(flipx)	flip |= GL_FLIP_H;
				if(flipy)	flip |= GL_FLIP_V; 

				glSpriteScaleXY( (x-camera_x)*scale_x, (y-camera_y)*scale_y,cosLerp(0)*scale_x,cosLerp(0)*scale_y, flip, &Atlas[sprite]);
			}
		} break;
		case CELESTE_P8_BTN: { //btn(b)
			int b = INT_ARG();
			assert(b >= 0 && b <= 5); 
			RET_BOOL(buttons_state & (1 << b));
		} break;
		case CELESTE_P8_SFX: { //sfx(id)
			int id = INT_ARG();
		
			if (id < (sizeof snd))//TODO / (sizeof*snd) && snd[id])
				mmEffectEx(&snd[id]);
		} break;
		case CELESTE_P8_PAL: { //pal(a,b)
			int a = INT_ARG();
			int b = INT_ARG();
			if (a >= 0 && a < 16 && b >= 0 && b < 16) {
				//swap palette colors
				palette[a] = atlasPal[b];//base_palette[b];
			}
		} break;
		case CELESTE_P8_PAL_RESET: { //pal()
			ResetPalette();
		} break;
		case CELESTE_P8_CIRCFILL: { //circfill(x,y,r,col)
			int cx = INT_ARG() - camera_x;
			int cy = INT_ARG() - camera_y;
			int r = INT_ARG();
			int col = INT_ARG();

			unsigned short realcolor = getcolor(col);

			if (r <= 1) {
				glBoxFilled(scale_x*(cx-1), scale_y*(cy  ), (scale_x*(cx-1))+scale_x*3, (scale_y*(cy  ))+scale_y  , realcolor);
				glBoxFilled(scale_x*(cx  ), scale_y*(cy-1), (scale_x*(cx  ))+scale_x  , (scale_y*(cy-1))+scale_y*3, realcolor);
			} else if (r <= 2) {
				glBoxFilled(scale_x*(cx-2), scale_y*(cy-1), (scale_x*(cx-2))+scale_x*5, (scale_y*(cy-1))+scale_y*3, realcolor);
				glBoxFilled(scale_x*(cx-1), scale_y*(cy-2), (scale_x*(cx-1))+scale_x*3, (scale_y*(cy-2))+scale_y*5, realcolor);
			} else if (r <= 3) {
				glBoxFilled(scale_x*(cx-3), scale_y*(cy-1), (scale_x*(cx-3))+scale_x*7, (scale_y*(cy-1))+scale_y*3, realcolor);
				glBoxFilled(scale_x*(cx-1), scale_y*(cy-3), (scale_x*(cx-1))+scale_x*3, (scale_y*(cy-3))+scale_y*7, realcolor);
				glBoxFilled(scale_x*(cx-2), scale_y*(cy-2), (scale_x*(cx-2))+scale_x*5, (scale_y*(cy-2))+scale_y*5, realcolor);
			} else { //i dont think the game uses this
				int f = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
				int ddFx = 1; //step x
				int ddFy = -2 * r; //step y
				int x = 0;
				int y = r;

				//this algorithm doesn't account for the diameters
				//so we have to set them manually
				p8_line(cx,cy-y, cx,cy+r, col);
				p8_line(cx+r,cy, cx-r,cy, col);

				while (x < y) {
					if (f >= 0) {
						y--;
						ddFy += 2;
						f += ddFy;
					}
					x++;
					ddFx += 2;
					f += ddFx;

					//build our current arc
					p8_line(cx+x,cy+y, cx-x,cy+y, col);
					p8_line(cx+x,cy-y, cx-x,cy-y, col);
					p8_line(cx+y,cy+x, cx-y,cy+x, col);
					p8_line(cx+y,cy-x, cx-y,cy-x, col);
				}
			}
		} break;
		case CELESTE_P8_PRINT: { //print(str,x,y,col)
			const char* str = va_arg(args, const char*);
			int x = INT_ARG() - camera_x;
			int y = INT_ARG() - camera_y;
			int col = INT_ARG() % 16;

			if (!strcmp(str, "x+c")) {
				//this is confusing, as 3DS uses a+b button, so use this hack to make it more appropiate
				str = "a+b";
			}

			p8_print(str,x,y,col);
		} break;
		case CELESTE_P8_RECTFILL: { //rectfill(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_rectfill(x0,y0,x1,y1,col);
		} break;
		case CELESTE_P8_LINE: { //line(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_line(x0,y0,x1,y1,col);
		} break;
		case CELESTE_P8_MGET: { //mget(tx,ty)
			int tx = INT_ARG();
			int ty = INT_ARG();

			RET_INT(tilemap_data[tx+ty*128]);
		} break;
		case CELESTE_P8_CAMERA: { //camera(x,y)
			if (enable_screenshake) {
				camera_x = INT_ARG();
				camera_y = INT_ARG();
			}
		} break;
		case CELESTE_P8_FGET: { //fget(tile,flag)
			int tile = INT_ARG();
			int flag = INT_ARG();

			RET_INT(gettileflag(tile, flag));
		} break;
		case CELESTE_P8_MAP: { //map(mx,my,tx,ty,mw,mh,mask)
			int mx = INT_ARG(), my = INT_ARG();
			int tx = INT_ARG(), ty = INT_ARG();
			int mw = INT_ARG(), mh = INT_ARG();
			int mask = INT_ARG();
			
			for (int x = 0; x < mw; x++) {
				for (int y = 0; y < mh; y++) {
					int tile = tilemap_data[x + mx + (y + my)*128];
					//hack
					if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask-1 : mask)) {						
						//glSpriteScaleXY(x*8*scale_x, y*8*scale_y,cosLerp(0)*scale_x,cosLerp(0)*scale_y, GL_FLIP_NONE, &Atlas[tile]);
						glSpriteScaleXY((tx+x*8-camera_x)*scale_x, (ty+y*8-camera_y)*scale_y,cosLerp(0)*scale_x,cosLerp(0)*scale_y, GL_FLIP_NONE, &Atlas[tile]);
					}
				}
			}
		} break;
	}

	end:
	va_end(args);
	return ret;
}

static void mainLoop(void) {
	glBegin2D();
	scanKeys();

	int keys = keysHeld();
	int keys_down = keysDown();
		
	static int reset_input_timer = 0;
	//select+start+y to reset
	if (initial_game_state != NULL && (keys & (KEY_START + KEY_SELECT + KEY_Y)))
	{
		reset_input_timer++;
		if (reset_input_timer >= 30) {
			reset_input_timer=0;
			//reset
			OSDset("reset");
			paused = 0;
			Celeste_P8_load_state(initial_game_state);
			Celeste_P8_set_rndseed(8);//TODO (unsigned)(time(NULL) + SDL_GetTicks()));
			//Mix_HaltChannel(-1);
			//Mix_HaltMusic();
			Celeste_P8_init();
		}
	} else reset_input_timer = 0;


	if(keys_down & KEY_R)
	{
		game_state = game_state ? game_state : malloc(Celeste_P8_get_state_size());
		if (game_state) {
			OSDset("save state");
			Celeste_P8_save_state(game_state);
			//TODO 
			//game_state_music = current_music;
		}
	}

	if(keys_down & KEY_L)
	{
		if (game_state) {
		OSDset("load state");
		//if (paused) paused = 0, Mix_Resume(-1), Mix_ResumeMusic();
		Celeste_P8_load_state(game_state);
		/*TODO
		if (current_music != game_state_music) {
			Mix_HaltMusic();
			current_music = game_state_music;
			if (game_state_music) Mix_PlayMusic(game_state_music, -1);
		}*/
		}
	}
	if(keys_down & KEY_X)
	{
		printf("floor 1.5: %f\n",floorf(1.5));
	}

	/*int prev_keys = keys;
	keys = keysHeld();

	if (!((prev_keys >> PSEUDO_BTN_PAUSE) & 1)
	 && (keys >> PSEUDO_BTN_PAUSE) & 1) {
		goto toggle_pause;
	}

	if (!((prev_keys >> PSEUDO_BTN_EXIT) & 1)
	 && (keys >> PSEUDO_BTN_EXIT) & 1) {
		goto press_exit;
	}

	if (!((prev_keys >> PSEUDO_BTN_SAVE_STATE) & 1)
	 && (keys >> PSEUDO_BTN_SAVE_STATE) & 1) {
		goto save_state;
	}

	if (!((prev_keys >> PSEUDO_BTN_LOAD_STATE) & 1)
	 && (keys >> PSEUDO_BTN_LOAD_STATE) & 1) {
		goto load_state;
	}*/

	//Todo: other control events
	/*while (SDL_PollEvent(&ev)) switch (ev.type) {
		case SDL_QUIT: running = 0; break;
		case SDL_KEYDOWN: {
			if SDL_MAJOR_VERSION >= 2
			if (ev.key.repeat) break; //no key repeat
			endif
			if (ev.key.keysym.sym == SDLK_ESCAPE) { //do pause
				toggle_pause:
				if (paused) Mix_Resume(-1), Mix_ResumeMusic(); else Mix_Pause(-1), Mix_PauseMusic();
				paused = !paused;
				break;
			} else if (ev.key.keysym.sym == SDLK_DELETE) { //exit
				press_exit:
				running = 0;
				break;
			} else if (ev.key.keysym.sym == SDLK_F11 && !(kbstate[SDLK_LSHIFT] || kbstate[SDLK_ESCAPE])) {
				if (SDL_WM_ToggleFullScreen(screen)) { //this doesn't work on windows..
					OSDset("toggle fullscreen");
				}
				screen = SDL_GetVideoSurface();
				break;
			} else if (0 && ev.key.keysym.sym == SDLK_5) {
				Celeste_P8__DEBUG();
				break;
			} else if (ev.key.keysym.sym == SDLK_s && kbstate[SDLK_LSHIFT]) { //save state
				save_state:
				game_state = game_state ? game_state : SDL_malloc(Celeste_P8_get_state_size());
				if (game_state) {
					OSDset("save state");
					Celeste_P8_save_state(game_state);
					game_state_music = current_music;
				}
				break;
			} else if (ev.key.keysym.sym == SDLK_d && kbstate[SDLK_LSHIFT]) { //load state
				load_state:
				if (game_state) {
					OSDset("load state");
					if (paused) paused = 0, Mix_Resume(-1), Mix_ResumeMusic();
					Celeste_P8_load_state(game_state);
					if (current_music != game_state_music) {
						Mix_HaltMusic();
						current_music = game_state_music;
						if (game_state_music) Mix_PlayMusic(game_state_music, -1);
					}
				}
				break;
			} else if ( //toggle screenshake (e / L+R)
		ifdef _3DS
					(ev.key.keysym.sym == SDLK_d && kbstate[SDLK_s]) || (ev.key.keysym.sym == SDLK_s && kbstate[SDLK_d])
		else
					ev.key.keysym.sym == SDLK_e
		endif
					) {
				enable_screenshake = !enable_screenshake;
				OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
			} break;
		}
	}*/
	buttons_state=0;
	if (keys & KEY_LEFT) 	buttons_state |= (1<<0);
	if (keys & KEY_RIGHT) 	buttons_state |= (1<<1);
	if (keys & KEY_UP) 		buttons_state |= (1<<2);
	if (keys & KEY_DOWN) 	buttons_state |= (1<<3);
	if (keys & KEY_A) 		buttons_state |= (1<<4);
	if (keys & KEY_B) 		buttons_state |= (1<<5);

	if (paused) {
		const int x0 = PICO8_W/2-3*4, y0 = 8;

		p8_rectfill(x0-1,y0-1, 6*4+x0+1,6+y0+1, 6);
		p8_rectfill(x0,y0, 6*4+x0,6+y0, 0);
		p8_print("paused", x0+1, y0+1, 7);
	} else {
		Celeste_P8_update();
		Celeste_P8_draw();
	}
	OSDdraw();

	glEnd2D();
	glFlush(0);
	
	swiWaitForVBlank();
	swiWaitForVBlank();
}

//---------------------------------------------------------------------
// main
//---------------------------------------------------------------------
int main(void) 
{
	//-----------------------------------------------------------------
	// Initialize the graphics engines
	//-----------------------------------------------------------------
	videoSetMode(MODE_5_3D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA( VRAM_A_TEXTURE );
	vramSetBankB( VRAM_B_TEXTURE );
	vramSetBankF(VRAM_F_TEX_PALETTE);  // Allocate VRAM bank for all the palettes	
	vramSetBankE(VRAM_E_MAIN_BG);	 // Main bg text/8bit bg. Bank E size == 64kb, exactly enough for 8bit * 256 * 192 + text layer

	vramSetBankC(VRAM_C_SUB_BG);

	glScreen2D();

	mmInitDefaultMem((mm_addr)soundbank_bin);

	// Our special palettes
	int OriginalPaletteID;	// Original palette

	// Generate another palette for our original image palette
	glGenTextures(1, &OriginalPaletteID);
	glBindTexture(0, OriginalPaletteID);
	glColorTableEXT(0,0,256,0,0,atlasPal);

	//-----------------------------------------------------------------
	// Initialize the console on bottom screen
	//-----------------------------------------------------------------
	const int tile_base = 0;
	const int map_base = 20;
	
	videoSetModeSub(MODE_0_2D);	

	PrintConsole *console = consoleInit(0,0, BgType_Text4bpp, BgSize_T_256x256, map_base, tile_base, false, false);

	ConsoleFont font;

	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 121;
	font.numColors =  fontPalLen / 2;
	font.bpp = 4;
	font.asciiOffset = 33;
	font.convertSingleColor = false;
	
	consoleSetFont(console, &font);

	/////////////////////////////////////////////////////////////////////////
	ResetPalette();
	printf("game state size %gkb\n", Celeste_P8_get_state_size()/1024.);
	printf("now loading...\n");

	/*{
		const unsigned char loading_bmp[] = {
			0x42,0x4d,0xca,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x82,0x00,
			0x00,0x00,0x6c,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x09,0x00,
			0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x48,0x00,
			0x00,0x00,0x23,0x2e,0x00,0x00,0x23,0x2e,0x00,0x00,0x02,0x00,
			0x00,0x00,0x02,0x00,0x00,0x00,0x42,0x47,0x52,0x73,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0x00,0x00,0x00,
			0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,
			0x00,0x00,0x66,0x3e,0xf1,0x24,0xf0,0x00,0x00,0x00,0x49,0x44,
			0x92,0x24,0x90,0x00,0x00,0x00,0x49,0x3c,0x92,0x24,0x90,0x00,
			0x00,0x00,0x49,0x04,0x92,0x24,0x90,0x00,0x00,0x00,0x46,0x38,
			0xf0,0x3c,0xf0,0x00,0x00,0x00,0x40,0x00,0x12,0x00,0x00,0x00,
			0x00,0x00,0xc0,0x00,0x10,0x00,0x00,0x00,0x00,0x00
		};
		SDL_Rect rc = {60, 60};
		SDL_BlitSurface(loading,NULL,screen,&rc);
	} skip_load:*/

	LoadData();


	int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);
	Celeste_P8_set_call_func(pico8emu);

	//for reset
	initial_game_state = malloc(Celeste_P8_get_state_size());
	if (initial_game_state) Celeste_P8_save_state(initial_game_state);

	//TODO: ADD RANDOM TIMER
	Celeste_P8_set_rndseed(8);//(unsigned)(time(NULL) + timer));

	Celeste_P8_init();

	printf("ready\n");


	//-----------------------------------------------------------------
	// main loop
	//-----------------------------------------------------------------
	while(running) 
	{
		mainLoop();
		oamUpdate(&oamSub);
	}

	if (game_state) free(game_state);
	if (initial_game_state) free(initial_game_state);

	return 0;
}
