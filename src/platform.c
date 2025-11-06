/*
 * platform.c
 *
 * Thanks to https://github.com/deltabeard/gameboy-c/ for 
 * the original source and to AtexBg for the linux3ds-lib
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdbool.h>

#include "gameboy.h"
#include "render.h"
#include "fps.h"
#include "input.h"

// emulator data
int running = 1;
int colorSetting = 0;
char buf[8];
int reload = 0;
u8   frameskip = 0;
u32  f0_ticks;
u32  f1_ticks;

// color schemes
u32 COLORS_Y[4] = {0xFFFFFFFF, 0x99999999, 0x44444444, 0x00000000};
u32 COLORS_O[4] = {0xFFFFFFFF, 0xFFFF9999, 0xFF444499, 0x00000000};
u32 COLORS_G[4] = {0xFFFFFFFF, 0xFF99FF99, 0xFF994444, 0x00000000};
u32 COLORS_B[4] = {0xFFFFFFFF, 0xFF9999FF, 0xFF449944, 0x00000000};
u32 COLORS_R[4] = {0xFFFFFFEE, 0xFFFFFF66, 0xFF444499, 0x00000000};
u32* color_map;

// gameboy color conversion
u32 ColorTo32(u16 cgb)
{
	u8 r = (cgb & 0x001F) << 3;// * 0xFF / 0x1F;
	u8 g = ((cgb >>  5) & 0x001F) << 3;// * 0xFF / 0x1F;
	u8 b = ((cgb >> 10) & 0x001F) << 3;// * 0xFF / 0x1F;

	//unused here
	//cy = (299*r + 587*g + 114*b) / 1000;
	//cb = (-16874*r - 33126*g + 50000*b + 12800000) / 100000;
	//cr = (50000*r - 41869*g - 8131*b + 12800000) / 100000;

	//*v0++ = *v1++ = (cy<<24) | (cb<<16) | (cy<<8) | cr;
	return 0xFF000000 | (r << 16) | (g << 8) | b;
}

// key mappings
#define NUM_KEYS    8
u32 KEYS[] =
{
	//TODO: add input ////TODO: remove this TODO saying "//TODO: add input" //////TODO: remove this TODO line saying "////TODO: remove this TODO saying "//TODO: add input"" beofre commiting on github
};

char  save_file[260];

// pointers
u8*   rom;
u32   rom_size;
u8*   save;
u32   save_size;
FILE* rom_f;
FILE* save_f;


int main(int argc, char **argv) {
	if(system("clear") == 0){}
	if(system("stty -echo") == 0){} //silent terminal
	int fbDevice = open("/dev/fb0", O_RDWR);
	if (fbDevice < 0){perror("open"); exit(1);}

	//allocate and mak front/back buffers for double buffering
	uint8_t *fb0 = mmap(NULL, 288000, PROT_READ | PROT_WRITE, MAP_SHARED, fbDevice, 0);
	uint8_t *backbuffer = malloc(288000);

	int     i, x, y;
	u8      j;
	u32     romread;
	u32     old_ticks;
	u32     new_ticks;
	int     delay;
	u32*    s;
	int     quit_seq;
	u32		fb[LCD_HEIGHT][LCD_WIDTH];
	char    reloadCmd[16];
	char	*rom_file = NULL;
	
	int c;

	while((c = getopt(argc, argv, "hm:f:")) != -1)
	{
		switch (c) {
			case 'h':
				printf("Usage: %s [-m magnification] -f GB_ROM\n", argv[0]);
				return 0;
			case 'f':
				rom_file = optarg;
				break;
			default:
				printf("?? getopt returned character code 0%o ??\n", c);
				return -1;
		}
	}

	if(rom_file == NULL)
	{
		printf("Please specify a file to load.\nUse -h to see help.\n");
		return -1;
	}

	// Load ROM file
	if((access(rom_file, F_OK) != -1) &&
			((rom_f = fopen(rom_file, "rb")) != NULL))
	{
		printf("%s: Opening %s.\n", __func__, rom_file);
		(rom_f = fopen(rom_file, "rb")) == NULL;
	}
	else
	{
		printf("%s: File \"%s\" not found.\n", __func__, rom_file);
		return -1;
	}

//19 lines of SDL code removed from here

	fseek(rom_f, 0, SEEK_END);
	rom_size = ftell(rom_f);
	rewind(rom_f);
	rom = (u8*)malloc(rom_size);
	for (i = 0; i < rom_size; i++)
		rom[i] = 0xFF;

	romread = fread(rom, sizeof(u8), rom_size, rom_f);
	fclose(rom_f);

	// Load SAVE file (if it exists)
	sprintf(save_file, "%s.sav", rom_file);
	save_size = GetSaveSize(rom);
	save = (u8*)malloc(save_size);
	save_f = fopen(save_file, "rb");
	if (save_f)
	{
		fseek(save_f, 0, SEEK_SET);
		if (fread(save, sizeof(u8), save_size, save_f) != save_size) {
    		perror("Error opening save file");
		}
		fclose(save_f);
	}

	// Start the emulator
	LoadROM(rom, rom_size, save, save_size);
	initInput();
	drawBackground(backbuffer);
	color_map = COLORS_Y;

	while (running){
		handleButtons();
		for (int k = 0; k < NUM_KEYS; k++) {
    		KeyRelease(k);
		}

		//custom input handler using linux3ds-lib
		if (isKeyPressed_RIGHT) KeyPress(0);
		if (isKeyPressed_LEFT)  KeyPress(1);
		if (isKeyPressed_UP)    KeyPress(2);
		if (isKeyPressed_DOWN)  KeyPress(3);
		if (isKeyPressed_A)     KeyPress(4); 
		if (isKeyPressed_B)     KeyPress(5); 
		if (isKeyPressed_X)     KeyPress(6); 
		if (isKeyPressed_START) KeyPress(7);

		clock_t start = clock();  //init start time of frame processing
		
		// emulate frame
		RunFrame();

		
		//and calculate fps
		int fps = getCurrentFPS();
		sprintf(buf, "FPS: %d", fps);

		if (gb_framecount == 0)
		{
			// convert colors
			if (cgb_enable)
				for (y = 0; y < LCD_HEIGHT; y++)
					for (x = 0; x < LCD_WIDTH; x++)
						fb[y][x] = ColorTo32(cgb_fb[y][x]);
			else
				for (y = 0; y < LCD_HEIGHT; y++)
					for (x = 0; x < LCD_WIDTH; x++)
						fb[y][x] = color_map[gb_fb[y][x] & 3];

			// render

			//frameskip is useless bc optimisations made
			//the emulator runs at 60fps
			if(isKeyPressed_L){running = 0; reload = 0;}
			if(isKeyPressed_R){running = 0; reload = 1;}
			
			//DRAWING AT SCREEN
		
			//FPS disabled bc segfaulting for some reason (still there for testing bruhh) //update : added again
			drawText(backbuffer, 230, 0 , buf, 255,255,255, 0,0,0); //display fps

			if(cgb_enable){
				drawFrameColor(backbuffer);
				memcpy(fb0, backbuffer, 288000);
			} else {
				drawFrame(backbuffer);
				memcpy(fb0, backbuffer, 288000);
			}
		}
	}

	// Save game before exit
	if (save_size)        {
		save_f = fopen(save_file, "wb");
		if (save_f)
		{
			fseek(save_f, 0, SEEK_SET);
			fwrite(save, 1, save_size, save_f);
			fclose(save_f);
		}
	}

	// Clean up
	if(system("stty echo") == 0){}
	closeInput();
	free(rom);
	free(save);
	snprintf(reloadCmd, sizeof(reloadCmd), "./gb3ds -f %s", rom_file);
	if(reload == 1){if(system(reloadCmd) == 0){}} //just a test dw

	return 0;
}

