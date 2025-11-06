#ifndef RENDER_H
#define RENDER_H

void drawFrame(uint8_t *fb0);
void drawFrameColor(uint8_t *fb0);
void drawBackground(uint8_t *fb0);
void drawText(uint8_t *fb0, int x0, int y0, const char *text, int8_t fb, uint8_t fg, uint8_t fr, uint8_t bb, uint8_t bg, uint8_t br);

#endif