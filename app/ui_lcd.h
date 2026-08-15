/*
 * UI LCD - Framebuffer Display Driver
 * For GEC6818 800x480 32bpp LCD
 */

#ifndef UI_LCD_H
#define UI_LCD_H

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  480

/* Colors (RGB 0x00RRGGBB) */
#define COLOR_BLACK       0x00000000
#define COLOR_WHITE       0x00FFFFFF
#define COLOR_RED         0x00FF0000
#define COLOR_GREEN       0x0000FF00
#define COLOR_BLUE        0x000000FF
#define COLOR_CYAN        0x0000FFFF
#define COLOR_YELLOW      0x00FFFF00
#define COLOR_ORANGE      0x00FF8000
#define COLOR_DARK_RED    0x00800000
#define COLOR_DARK_GREEN  0x00008000
#define COLOR_DARK_BLUE   0x00000080
#define COLOR_NAVY        0x00000040
#define COLOR_GRAY        0x00808080
#define COLOR_LIGHT_GRAY  0x00C0C0C0
#define COLOR_DARK_GRAY   0x00404040

/* Function prototypes */
int  lcd_init(void);
void lcd_close(void);
void lcd_clear(unsigned int color);
void lcd_draw_pixel(int x, int y, unsigned int color);
void lcd_draw_char(int x, int y, char c, unsigned int color, unsigned int bg_color);
void lcd_draw_string(int x, int y, const char *str, unsigned int color, unsigned int bg_color);
void lcd_fill_rect(int x, int y, int w, int h, unsigned int color);
void lcd_draw_rect(int x, int y, int w, int h, unsigned int color);
int  lcd_get_width(void);
int  lcd_get_height(void);

#endif /* UI_LCD_H */
