/*
 * UI LCD - Framebuffer Display Implementation
 * For GEC6818 800x480 32bpp LCD
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "ui_lcd.h"
#include "font_8x16.h"

#define FB_DEVICE "/dev/fb0"

static int  fb_fd = -1;
static unsigned int *fb_ptr = NULL;   /* mmap pointer */
static int  fb_width = 800;
static int  fb_height = 480;
static int  fb_line_len = 0;
static int  fb_pix_bytes = 4;         /* 32bpp = 4 bytes per pixel */

int lcd_init(void)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    /* Open framebuffer device */
    fb_fd = open(FB_DEVICE, O_RDWR);
    if (fb_fd < 0) {
        perror("open " FB_DEVICE);
        return -1;
    }

    /* Get variable screen info */
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO");
        close(fb_fd);
        fb_fd = -1;
        return -1;
    }

    /* Force 32bpp */
    vinfo.bits_per_pixel = 32;
    if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOPUT_VSCREENINFO");
        /* Non-fatal, continue */
    }

    /* Get fixed screen info */
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        perror("FBIOGET_FSCREENINFO");
        close(fb_fd);
        fb_fd = -1;
        return -1;
    }

    /* Re-query variable info after possible change */
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("FBIOGET_VSCREENINFO (2)");
        close(fb_fd);
        fb_fd = -1;
        return -1;
    }

    fb_width    = vinfo.xres;
    fb_height   = vinfo.yres;
    fb_line_len = finfo.line_length;
    fb_pix_bytes = vinfo.bits_per_pixel / 8;

    printf("LCD: %dx%d, %dbpp, line_len=%d\n",
           fb_width, fb_height, vinfo.bits_per_pixel, fb_line_len);

    /* mmap framebuffer */
    size_t screensize = fb_line_len * fb_height;
    fb_ptr = (unsigned int *)mmap(NULL, screensize,
                                  PROT_READ | PROT_WRITE, MAP_SHARED,
                                  fb_fd, 0);
    if (fb_ptr == MAP_FAILED) {
        perror("mmap framebuffer");
        close(fb_fd);
        fb_fd = -1;
        return -1;
    }

    return 0;
}

void lcd_close(void)
{
    if (fb_ptr != NULL) {
        size_t screensize = fb_line_len * fb_height;
        munmap(fb_ptr, screensize);
        fb_ptr = NULL;
    }
    if (fb_fd >= 0) {
        close(fb_fd);
        fb_fd = -1;
    }
}

void lcd_clear(unsigned int color)
{
    int x, y;
    for (y = 0; y < fb_height; y++) {
        for (x = 0; x < fb_width; x++) {
            fb_ptr[y * (fb_line_len / fb_pix_bytes) + x] = color;
        }
    }
}

void lcd_draw_pixel(int x, int y, unsigned int color)
{
    if (x < 0 || x >= fb_width || y < 0 || y >= fb_height)
        return;
    fb_ptr[y * (fb_line_len / fb_pix_bytes) + x] = color;
}

void lcd_draw_char(int x, int y, char c, unsigned int color, unsigned int bg_color)
{
    const unsigned char *glyph;
    int row, col;
    unsigned char bits;

    if (x + FONT_WIDTH > fb_width || y + FONT_HEIGHT > fb_height)
        return;

    glyph = font_get_char(c);

    for (row = 0; row < FONT_HEIGHT; row++) {
        bits = glyph[row];
        for (col = 0; col < FONT_WIDTH; col++) {
            if (bits & (0x80 >> col)) {
                fb_ptr[(y + row) * (fb_line_len / fb_pix_bytes) + (x + col)] = color;
            } else if (bg_color != COLOR_BLACK) {
                /* Draw background pixel */
                fb_ptr[(y + row) * (fb_line_len / fb_pix_bytes) + (x + col)] = bg_color;
            }
        }
    }
}

void lcd_draw_string(int x, int y, const char *str, unsigned int color, unsigned int bg_color)
{
    int start_x = x;

    while (*str) {
        if (*str == '\n') {
            y += FONT_HEIGHT + 2;
            x = start_x;
            str++;
            continue;
        }
        lcd_draw_char(x, y, *str, color, bg_color);
        x += FONT_WIDTH;
        if (x + FONT_WIDTH > fb_width) {
            x = start_x;
            y += FONT_HEIGHT + 2;
        }
        str++;
    }
}

void lcd_fill_rect(int x, int y, int w, int h, unsigned int color)
{
    int i, j;
    int row_stride = fb_line_len / fb_pix_bytes;

    /* Clipping */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > fb_width)  w = fb_width - x;
    if (y + h > fb_height) h = fb_height - y;
    if (w <= 0 || h <= 0) return;

    for (j = y; j < y + h; j++) {
        for (i = x; i < x + w; i++) {
            fb_ptr[j * row_stride + i] = color;
        }
    }
}

void lcd_draw_rect(int x, int y, int w, int h, unsigned int color)
{
    int row_stride = fb_line_len / fb_pix_bytes;
    int i;

    if (x < 0 || y < 0 || x + w > fb_width || y + h > fb_height)
        return;

    /* Top edge */
    for (i = x; i < x + w; i++)
        fb_ptr[y * row_stride + i] = color;
    /* Bottom edge */
    for (i = x; i < x + w; i++)
        fb_ptr[(y + h - 1) * row_stride + i] = color;
    /* Left edge */
    for (i = y; i < y + h; i++)
        fb_ptr[i * row_stride + x] = color;
    /* Right edge */
    for (i = y; i < y + h; i++)
        fb_ptr[i * row_stride + (x + w - 1)] = color;
}

int lcd_get_width(void)  { return fb_width; }
int lcd_get_height(void) { return fb_height; }
