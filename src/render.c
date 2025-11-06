#include <stdint.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>

#define LCD_WIDTH 160
#define LCD_HEIGHT 144
#define SCREEN_W 240
#define SCREEN_H 400
#define CHAR_H 8
#define CHAR_W 10
#define BYTES_PER_PIXEL 3
#define STRIDE 720

extern uint8_t gb_fb[LCD_HEIGHT][LCD_WIDTH];
extern uint16_t cgb_fb[LCD_HEIGHT][LCD_WIDTH];

#include "font.txt"

//static const uint8_t color_map[4][3] = {
//    {255, 255, 255}, //0- white
//    {192, 192, 192}, //1- soft gray
//    {96,  96,  96},  //2- dark gray
//    {0,   0,   0}    //3- black
//};

static const uint8_t color_map[4][3] = {
    {0xFF, 0xFF, 0xFF}, //0- white
    {0xFF, 0x84, 0x84}, //1- soft gray
    {0x94, 0x3A, 0x3A},  //2- dark gray
    {0x00, 0x00, 0x00}    //3- black
};

void drawFrame(uint8_t *backbuffer) {
    int offset_x = 47; //offset so it's centered
    int offset_y = 120; 

    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            uint8_t shade = gb_fb[LCD_HEIGHT - 1 - y][x] & 3;

            int dst_x = y + offset_x;       
            int dst_y = x + offset_y;

            if (dst_x < 0 || dst_x >= SCREEN_W || dst_y < 0 || dst_y >= SCREEN_H)
                continue;

            uint8_t *dst = backbuffer + dst_y * STRIDE + dst_x * BYTES_PER_PIXEL;
            dst[0] = color_map[shade][2]; // B
            dst[1] = color_map[shade][1]; // G
            dst[2] = color_map[shade][0]; // R
        }
    }
}

void drawFrameColor(uint8_t *backbuffer) {
    int offset_x = 47;
    int offset_y = 120;

    for (int y = 0; y < LCD_HEIGHT; y++) {
        for (int x = 0; x < LCD_WIDTH; x++) {
            uint16_t color = cgb_fb[LCD_HEIGHT - 1 - y][x]; 

            uint8_t r = (color & 0x1F) << 3;
            uint8_t g = ((color >> 5) & 0x1F) << 3;
            uint8_t b = ((color >> 10) & 0x1F) << 3;

            int dst_x = y + offset_x;
            int dst_y = x + offset_y;

            if (dst_x < 0 || dst_x >= SCREEN_W || dst_y < 0 || dst_y >= SCREEN_H)
                continue;

            uint8_t *dst = backbuffer + dst_y * STRIDE + dst_x * BYTES_PER_PIXEL;
            dst[0] = b;
            dst[1] = g;
            dst[2] = r;
        }
    }
}

const uint8_t (*getCharSprite(char c))[CHAR_W][3] {
    switch (c) {
        // Lookup table (kinda) to get every char
        // ADDING NON-ASCII CHARS WILL BREAK THE CODE!
        case 'A': return a;
        case 'B': return b;
        case 'C': return c_char; // not just "c" bc it will mess up with libc types
        case 'D': return d;
        case 'E': return e;
        case 'F': return f;
        case 'G': return g;
        case 'H': return h;
        case 'I': return i;
        case 'J': return j;
        case 'K': return k;
        case 'L': return l;
        case 'M': return m;
        case 'N': return n;
        case 'O': return o;
        case 'P': return p;
        case 'Q': return q;
        case 'R': return r;
        case 'S': return s;
        case 'T': return t;
        case 'U': return u;
        case 'V': return v;
        case 'W': return w;
        case 'X': return x;
        case 'Y': return y;
        case 'Z': return z;
        //numbers
        case '0': return num0;
        case '1': return num1;
        case '2': return num2;
        case '3': return num3;
        case '4': return num4;
        case '5': return num5;
        case '6': return num6;
        case '7': return num7;
        case '8': return num8;
        case '9': return num9;
        // Special chars
        case '!': return exclamation;
        case '?': return question;
        case ' ': return space;
        case '*': return star;
        case '/': return slash;
        case '_': return underscore;
        case '=': return equal;
        case ':': return colon;
        case '&': return and;
        case '\\': return backslash;
        case '#': return hashtag;
        case '^': return hat;
        case '-': return minus;
        case '(': return parh1;
        case ')': return parh2;
        case '%': return percent;
        case '|': return pipe_;
        case '+': return plus;
        case '"': return quote;
        case '.': return dot;
        case ';': return semicolon;
        case ',': return comma;
        case '~': return tilde;
        // chars ←, →, ↑ and ↓ can be used, but only as {, [, ] and }
        case '{': return left;
        case '[': return right;
        case ']': return up;
        case '}': return down;
        // battery icons
        case 'e': return bat_empty; //empty
        case 'l': return bat_low; //low
        case 'm': return bat_med; //medium
        case 'h': return bat_high; //high
        case 'f': return bat_full; //full

        default: return NULL; // if unknown char
    }
}

void drawText(uint8_t *fb0, int x0, int y0, const char *text, int8_t fb, uint8_t fg, uint8_t fr, uint8_t bb, uint8_t bg, uint8_t br){
    int txt_cursor_x = x0;
    int txt_cursor_y = y0;

    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];

        if (c == '\n') {
            txt_cursor_x -= CHAR_W;
            txt_cursor_y = y0;
            continue;
        }

        const uint8_t (*sprite)[CHAR_W][3] = getCharSprite(c);
        if (!sprite) continue;

        // calculate char start in framebuffer
        uint8_t *dst_line = fb0 + (txt_cursor_y * 720 + txt_cursor_x * 3);

        for (int y = 0; y < CHAR_H; y++) {
            uint8_t *dst = dst_line;
            const uint8_t (*src_row)[3] = sprite[y];

            for (int x = 0; x < CHAR_W; x++) {
                const uint8_t *pix = src_row[x];

                // test intensity instead of 3 color checks
                uint8_t intensity = pix[0] | pix[1] | pix[2];
                if (intensity) {
                    dst[0] = fr; dst[1] = fg; dst[2] = fb;
                } else {
                    dst[0] = br; dst[1] = bg; dst[2] = bb;
                }

                dst += 3;
            }

            dst_line += 720;
        }

        txt_cursor_y += CHAR_H;
    }
}

#include "background.array"

void drawBackground(uint8_t *fb0){
    memcpy(fb0, background, sizeof(background));
}

